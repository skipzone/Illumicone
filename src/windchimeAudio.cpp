/*
    This file is part of Illumicone.

    Illumicone is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Illumicone is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Illumicone.  If not, see <http://www.apache.org/licenses/>.
*/

#include <atomic>
#include <chrono>
#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <portaudio.h>

#include "illumiconeWidgetTypes.h"
#include "Log.h"
#include "WindchimeAudioEngine.h"

using namespace std;

Log logger;
static atomic<bool> keepRunning(true);

// The process is kept alive until the user sends SIGINT or SIGTERM. This
// handler simply flips a flag so the main loop can exit cleanly and shut down
// PortAudio and the UDP sockets in a controlled order.
static void signalHandler(int signum)
{
    (void) signum;
    keepRunning = false;
}


// WidgetUdpReceiver listens on one UDP socket per widget ID and forwards each
// valid packet into the shared WindchimeAudioEngine. The receiver is a thin
// transport layer: it does not interpret the payload beyond checking the size
// and passing the measurements through to noteOn().
class WidgetUdpReceiver
{
    public:

        WidgetUdpReceiver(unsigned int widgetPortBase, WindchimeAudioEngine& engine)
            : widgetPortBase(widgetPortBase)
            , engine(engine)
            , stopRequested(false)
        {
        }

        virtual ~WidgetUdpReceiver()
        {
            stop();
        }

        // Open one socket for each widget and start a receive thread for each one.
        // This keeps the code simple while still allowing all widgets to generate
        // audio events independently.
        bool start()
        {
            for (unsigned int widgetId = 1; widgetId < 32; ++widgetId) {
                int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
                if (sockfd < 0) {
                    logger.logMsg(LOG_ERR, errno, "Failed to create socket for widget %u.", widgetId);
                    stop();
                    return false;
                }

                sockaddr_in addr;
                memset(&addr, 0, sizeof(addr));
                addr.sin_family = AF_INET;
                addr.sin_addr.s_addr = htonl(INADDR_ANY);
                addr.sin_port = htons(widgetPortBase + widgetId);

                if (::bind(sockfd, (struct sockaddr*) &addr, sizeof(addr)) < 0) {
                    logger.logMsg(LOG_ERR, errno, "Failed to bind socket for widget %u.", widgetId);
                    close(sockfd);
                    stop();
                    return false;
                }

                sockets.push_back(sockfd);
                threads.emplace_back(&WidgetUdpReceiver::receiveLoop, this, sockfd, widgetId);
            }

            return true;
        }

        // Request shutdown, close all sockets, and join every receive thread so
        // the program exits only after all worker threads have stopped.
        void stop()
        {
            stopRequested = true;
            for (int sockfd : sockets) {
                if (sockfd >= 0) {
                    shutdown(sockfd, SHUT_RDWR);
                    close(sockfd);
                }
            }

            for (auto& thread : threads) {
                if (thread.joinable()) {
                    thread.join();
                }
            }

            threads.clear();
            sockets.clear();
        }

    private:

        // Each thread blocks on recvfrom() for one widget's UDP port. When data
        // arrives, we validate the packet size and pass the payload directly into
        // the synthesis engine.
        void receiveLoop(int sockfd, unsigned int widgetId)
        {
            sockaddr_in cliaddr;
            socklen_t len = sizeof(cliaddr);
            UdpPayload payload;

            while (!stopRequested) {
                ssize_t bytesReceived = recvfrom(sockfd, &payload, sizeof(payload), 0,
                                                 (struct sockaddr*) &cliaddr, &len);
                if (bytesReceived < 0) {
                    if (stopRequested) {
                        break;
                    }
                    logger.logMsg(LOG_ERR, errno, "Error receiving UDP payload for widget %u.", widgetId);
                    continue;
                }

                if (bytesReceived < static_cast<ssize_t>(sizeof(UdpPayload))) {
                    continue;
                }

                engine.noteOn(payload.id,
                              payload.channel,
                              payload.position,
                              payload.velocity,
                              payload.isActive != 0);
            }
        }

        unsigned int widgetPortBase;   // Base UDP port used to derive per-widget ports.
        WindchimeAudioEngine& engine; // Shared synthesizer that all receive threads update.
        atomic<bool> stopRequested;    // Cooperative shutdown flag used by worker threads.
        vector<int> sockets;           // One socket per widget, kept alive until stop().
        vector<thread> threads;        // One receive thread per socket.
};


// PortAudio callback invoked on the real-time audio thread. The callback does
// not do any blocking work or allocation, and it simply asks the engine to fill
// the output buffer with the current synthesized audio.
static int paCallback(const void* inputBuffer,
                      void* outputBuffer,
                      unsigned long framesPerBuffer,
                      const PaStreamCallbackTimeInfo* timeInfo,
                      PaStreamCallbackFlags statusFlags,
                      void* userData)
{
    (void) inputBuffer;
    (void) timeInfo;
    (void) statusFlags;

    WindchimeAudioEngine* engine = static_cast<WindchimeAudioEngine*>(userData);
    float* out = static_cast<float*>(outputBuffer);
    engine->render(out, framesPerBuffer);
    return paContinue;
}


// Print the command-line usage information for the standalone audio program.
static void usage(const char* programName)
{
    cerr << "Usage: " << programName << " [--widget-port-base <base>]" << endl;
}


int main(int argc, char** argv)
{
    // Default to the existing widget test port range. Users can override the
    // base so the program can be pointed at a different UDP configuration.
    unsigned int widgetPortBase = 4200;

    for (int i = 1; i < argc; ++i) {
        string arg(argv[i]);
        if (arg == "--widget-port-base" && i + 1 < argc) {
            widgetPortBase = static_cast<unsigned int>(strtoul(argv[++i], NULL, 10));
        }
        else if (arg == "-h" || arg == "--help") {
            usage(argv[0]);
            return 0;
        }
        else {
            cerr << "Unknown option: " << arg << endl;
            usage(argv[0]);
            return 1;
        }
    }

    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    logger.startLogging("windchimeAudio", Log::LogTo::console);

    PaError err = Pa_Initialize();
    if (err != paNoError) {
        logger.logMsg(LOG_ERR, "PortAudio initialization failed: %s", Pa_GetErrorText(err));
        return 1;
    }

    // The audio engine is shared between the UDP receiver and the PortAudio
    // callback, so it must outlive both the receiver and the stream.
    WindchimeAudioEngine audioEngine;
    WidgetUdpReceiver receiver(widgetPortBase, audioEngine);
    if (!receiver.start()) {
        Pa_Terminate();
        logger.stopLogging();
        return 1;
    }

    PaStream* stream = nullptr;
    err = Pa_OpenDefaultStream(&stream,
                               0,
                               2,
                               paFloat32,
                               48000,
                               512,
                               paCallback,
                               &audioEngine);
    if (err != paNoError) {
        logger.logMsg(LOG_ERR, "Failed to open default audio stream: %s", Pa_GetErrorText(err));
        receiver.stop();
        Pa_Terminate();
        logger.stopLogging();
        return 1;
    }

    err = Pa_StartStream(stream);
    if (err != paNoError) {
        logger.logMsg(LOG_ERR, "Failed to start audio stream: %s", Pa_GetErrorText(err));
        Pa_CloseStream(stream);
        receiver.stop();
        Pa_Terminate();
        logger.stopLogging();
        return 1;
    }

    logger.logMsg(LOG_INFO, "Windchime audio is running on widget port base %u.", widgetPortBase);

    while (keepRunning) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    if (stream != nullptr) {
        Pa_StopStream(stream);
        Pa_CloseStream(stream);
    }

    receiver.stop();
    Pa_Terminate();
    logger.stopLogging();

    return 0;
}
