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
    along with Illumicone.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <RF24/RF24.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>

#include "ConfigReader.h"
#include "illumiconeUtility.h"
#include "illumiconeWidgetTypes.h"
#include "log.h"
#include "WidgetId.h"


using namespace std;


// TODO 8/3/2017 ross:  Get this from config.
static string lockFilePath = "/tmp/widgetRcvr.lock";

static ConfigReader config;
static string patconIpAddress;
static unsigned int widgetPortNumberBase;

static struct sockaddr_in widgetSockAddr[16];
static int widgetSock[16];



/***********************
 * Radio Configuration *
 ***********************/

// Radio CE Pin, CSN Pin, SPI Speed
// See http://www.airspayce.com/mikem/bcm2835/group__constants.html#ga63c029bd6500167152db4e57736d0939
// and the related enumerations for pin information.  (That's some pretty useful shit right there, Maynard.)
// Raspberry Pi B+:  CE connected to GPIO25 on J8-22, CSN connected to CE0
RF24 radio(RPI_BPLUS_GPIO_J8_22, RPI_BPLUS_GPIO_J8_24, BCM2835_SPI_SPEED_8MHZ);

// We're using dynamic payload size, but we still need to know what the largest can be.
constexpr uint8_t maxPayloadSize = 32;

constexpr uint8_t readPipeAddresses[][6] = {"0wdgt", "1wdgt", "2wdgt", "3wdgt", "4wdgt", "5wdgt"};
constexpr int numReadPipes = sizeof(readPipeAddresses) / (sizeof(uint8_t) * 6);

// nRF24 frequency range:  2400 to 2525 MHz (channels 0 to 125)
// ISM: 2400-2500;  ham: 2390-2450
// WiFi ch. centers: 1:2412, 2:2417, 3:2422, 4:2427, 5:2432, 6:2437, 7:2442,
//                   8:2447, 9:2452, 10:2457, 11:2462, 12:2467, 13:2472, 14:2484
constexpr uint8_t rfChannel = 84;

// RF24_PA_MIN = -18 dBm, RF24_PA_LOW = -12 dBm, RF24_PA_HIGH = -6 dBm, RF24_PA_MAX = 0 dBm
constexpr rf24_pa_dbm_e rfPowerLevel = RF24_PA_MAX;

// RF24_250KBPS or RF24_1MBPS
constexpr rf24_datarate_e dataRate = RF24_250KBPS;

constexpr uint8_t txRetryDelayMultiplier = 15;  // 250 us additional delay multiplier (0-15)
constexpr uint8_t txMaxRetries = 15;            // max retries (0-15)

// RF24_CRC_DISABLED, RF24_CRC_8, or RF24_CRC_16
constexpr rf24_crclength_e crcLength = RF24_CRC_16;


/*********************
 * UDP Communication *
 *********************/

bool openUdpPort(WidgetId widgetId)
{
    unsigned int widgetIdNumber = widgetIdToInt(widgetId);
    unsigned int portNumber = widgetPortNumberBase + widgetIdNumber;

    logMsg(LOG_INFO, "Creating and binding socket for " + widgetIdToString(intToWidgetId(widgetIdNumber)));

    memset(&widgetSockAddr[widgetIdNumber], 0, sizeof(struct sockaddr_in));

    widgetSockAddr[widgetIdNumber].sin_family = AF_INET;
    widgetSockAddr[widgetIdNumber].sin_addr.s_addr = htonl(INADDR_ANY);
    widgetSockAddr[widgetIdNumber].sin_port = htons(0);

    if ((widgetSock[widgetIdNumber] = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        int errNum = errno;
        logMsg(LOG_ERR,
               "Failed to create socket for " + widgetIdToString(intToWidgetId(widgetIdNumber))
               + ".  " + string(strerror(errNum)) + " (" + to_string(errNum) + ")");
        return false;
    }

    if (::bind(widgetSock[widgetIdNumber], (struct sockaddr *) &widgetSockAddr[widgetIdNumber], sizeof(struct sockaddr_in)) < 0) {
        int errNum = errno;
        logMsg(LOG_ERR,
               "bind failed for " + widgetIdToString(intToWidgetId(widgetIdNumber))
               + ".  " + string(strerror(errNum)) + " (" + to_string(errNum) + ")");
        return false;
    }

    logMsg(LOG_INFO, "Setting address to " + patconIpAddress + ":" + to_string(portNumber));

    inet_pton(AF_INET, patconIpAddress.c_str(), &widgetSockAddr[widgetIdNumber].sin_addr.s_addr);
    widgetSockAddr[widgetIdNumber].sin_port = htons(portNumber);

    return true;
}


bool sendUdp(const UdpPayload& payload)
{
    ssize_t bytesSentCount = sendto(widgetSock[payload.id],
                                    &payload,
                                    sizeof(payload),
                                    0,
                                    (struct sockaddr *) &widgetSockAddr[payload.id],
                                    sizeof(struct sockaddr_in));

    if (bytesSentCount != sizeof(payload)) {
        logMsg(LOG_ERR,
               "UPD payload size is " + to_string(sizeof(payload))
               + " but " + to_string(bytesSentCount) + " bytes were sent.");
        return false;
    }
    //logMsg(LOG_DEBUG, "Sent " to_string(bytesSentCount) + " byte payload via UDP.");

    return true;
}


/********************
 * Payload Handlers *
 ********************/

void handleStressTestPayload(const StressTestPayload* payload, unsigned int payloadSize)
{
    if (payloadSize != sizeof(StressTestPayload)) {
        logMsg(LOG_ERR,
               "Got StressTestPayload payload with size " + to_string(payloadSize)
               + " but size " + to_string(sizeof(StressTestPayload)) + " was expected.");
        return;
    }

    logMsg(LOG_INFO,
           "Got stress test payload; Id = " + to_string((int) payload->widgetHeader.id)
           + ", " + string(payload->widgetHeader.isActive ? "active  " : "inactive")
           + ", ch = " + to_string((int) payload->widgetHeader.channel)
           + ", seq = " + to_string(payload->payloadNum)
           + ", fails = " + to_string(payload->numTxFailures)
           + " (" + to_string(payload->numTxFailures * 100 / payload->payloadNum) + "% )");

    UdpPayload udpPayload;
    udpPayload.id       = payload->widgetHeader.id;
    udpPayload.channel  = payload->widgetHeader.channel;
    udpPayload.isActive = payload->widgetHeader.isActive;
    udpPayload.position = payload->payloadNum;
    udpPayload.velocity = payload->numTxFailures;

    sendUdp(udpPayload);
}


void handlePositionVelocityPayload(const PositionVelocityPayload* payload, unsigned int payloadSize)
{
    if (payloadSize != sizeof(PositionVelocityPayload)) {
        logMsg(LOG_ERR,
               "Got PositionVelocityPayload payload with size " + to_string(payloadSize)
               + " but size " + to_string(sizeof(PositionVelocityPayload)) + " was expected.");
        return;
    }

    logMsg(LOG_INFO,
           "Got Got position+velocity payload; Id = " + to_string((int) payload->widgetHeader.id)
           + ", " + string(payload->widgetHeader.isActive ? "active  " : "inactive")
           + ", ch = " + to_string((int) payload->widgetHeader.channel)
           + ", position = " + to_string(payload->position)
           + ", velocity = " + to_string(payload->velocity));

    UdpPayload udpPayload;
    udpPayload.id       = payload->widgetHeader.id;
    udpPayload.channel  = payload->widgetHeader.channel;
    udpPayload.isActive = payload->widgetHeader.isActive;
    udpPayload.position = payload->position;
    udpPayload.velocity = payload->velocity;

    sendUdp(udpPayload);
}


void handleMeasurementVectorPayload(const MeasurementVectorPayload* payload, unsigned int payloadSize)
{
    if (payloadSize < (1 + sizeof(int16_t))) {
        logMsg(LOG_ERR, "Got MeasurementVectorPayload without any data.");
        return;
    }
    // TODO 8/3/2017 ross:  might be a good idea to make sure payloadSize is odd

    int numMeasurements = (payloadSize - 1) / sizeof(int16_t);

    logMsg(LOG_INFO,
           "Got measurement vector payload; Id = " + to_string((int) payload->widgetHeader.id)
           + ", " + string(payload->widgetHeader.isActive ? "active  " : "inactive")
           + ", ch = " + to_string((int) payload->widgetHeader.channel)
           + ", numMeasurements = " + to_string(numMeasurements));
    stringstream sstr;
    for (int i = 0; i < numMeasurements; ++i) {
        sstr << "  " << setfill(' ') << setw(6) << payload->measurements[i];
    }
    logMsg(LOG_INFO, "Measurements:" + sstr.str());

    // The rainstick widget sends 7 position measurements.  We'll map them to
    // channels 0 through 6.
    if (payload->widgetHeader.id == widgetIdToInt(WidgetId::rainstick)) {
        for (unsigned int i = 0; i < 7; ++i) {
            UdpPayload udpPayload;
            udpPayload.id       = payload->widgetHeader.id;
            udpPayload.channel  = i;
            udpPayload.isActive = payload->widgetHeader.isActive;
            udpPayload.position = payload->measurements[i];
            udpPayload.velocity = 0;
            sendUdp(udpPayload);
        }
    }
}


void handleCustomPayload(const CustomPayload* payload, unsigned int payloadSize)
{
    if (payloadSize < 2) {
        logMsg(LOG_ERR, + "Got CustomPayload without any data.");
        return;
    }

    int bufLen = payloadSize - 1;

    logMsg(LOG_INFO,
           "Got custom payload; Id = " + to_string((int) payload->widgetHeader.id)
           + ", " + string(payload->widgetHeader.isActive ? "active  " : "inactive")
           + ", ch = " + to_string((int) payload->widgetHeader.channel)
           + ", bufLen = " + to_string(bufLen));
    stringstream sstr;
    for (int i = 0; i < bufLen; ++i) {
        sstr << hex << (int) payload->buf[i] << " ";
    }
    logMsg(LOG_INFO, "Contents:  " + sstr.str());

    // TODO 8/3/2017 ross:  Do transformation for contortOMatic here.
}


/*********************************************
 * Initialization, Run Loop, and Entry Point *
 *********************************************/

bool readConfig(const string& configFileName)
{
    if (!config.readConfigurationFile(configFileName)) {
        return false;
    }

    patconIpAddress = config.getPatconIpAddress();
    if (patconIpAddress.empty()) {
        return false;
    }

    widgetPortNumberBase = config.getWidgetPortNumberBase();
    if (widgetPortNumberBase == 0) {
        return false;
    }

    return true;
}


bool openUdpPorts()
{
    bool retval = 
           openUdpPort(WidgetId::eye)
        && openUdpPort(WidgetId::spinnah)
        && openUdpPort(WidgetId::bells)
        && openUdpPort(WidgetId::rainstick)
        && openUdpPort(WidgetId::schroedersPlaything)
        && openUdpPort(WidgetId::triObelisk)
        && openUdpPort(WidgetId::pump)
        && openUdpPort(WidgetId::contortOMatic)
        && openUdpPort(WidgetId::fourPlay42)
        && openUdpPort(WidgetId::fourPlay43)
        && openUdpPort(WidgetId::buckNorris);

    return retval;
}


bool configureRadio()
{
    if (!radio.begin()) {
        logMsg(LOG_ERR, "radio.begin failed.");
        return false;
    }

    radio.setPALevel(rfPowerLevel);
    radio.setRetries(txRetryDelayMultiplier, txMaxRetries);
    radio.setDataRate(dataRate);
    radio.setChannel(rfChannel);
    radio.setAutoAck(true);
    radio.enableDynamicPayloads();
    radio.setCRCLength(crcLength);

    for (uint8_t i = 0; i < numReadPipes; ++i) {
        radio.openReadingPipe(i, readPipeAddresses[i]);
    }

    radio.printDetails();

    return true;
}


void runLoop()
{

    while (1) {

        uint8_t pipeNum;
        while(radio.available(&pipeNum)) {

            unsigned int payloadSize = radio.getDynamicPayloadSize();
            if (payloadSize == 0) {
                logMsg(LOG_ERR, "Got invalid packet (payloadSize = 0).");
                continue;
            }
            if (payloadSize > maxPayloadSize) {
                logMsg(LOG_ERR, "Got unsupported payload size " + to_string(payloadSize));
                // RF24 is supposed to do a Flush_RX command and return 0 for
                // the size if an invalid payload length is detected.  It
                // apparently didn't do that.  Who knows what we're supposed
                // to do now.  We'll try turning receive off and on to clear
                // the rx buffers and start over.
                radio.stopListening();
                delay(100);
                radio.startListening();
                continue;
            }

            uint8_t payload[payloadSize];
            radio.read(payload, payloadSize);

            stringstream sstr;

            switch(pipeNum) {

                case 0:
                    handleStressTestPayload((StressTestPayload*) payload, payloadSize);
                    break;

                case 1:
                    handlePositionVelocityPayload((PositionVelocityPayload*) payload, payloadSize);
                    break;

                case 2:
                    handleMeasurementVectorPayload((MeasurementVectorPayload*) payload, payloadSize);
                    break;

                case 3:
                case 4:
                    for (int i = 0; i < maxPayloadSize; ++i) {
                        sstr << hex << (int) payload[i] << " ";
                    }
                    logMsg(LOG_ERR,
                           "Got payload with size " + to_string(payloadSize)
                           + " via unsupported pipe " + to_string((int) pipeNum)
                           + ".  Contents:  " + sstr.str());
                    break;

                case 5:
                    handleCustomPayload((CustomPayload*) payload, payloadSize);
                    break;

                default:
                    for (int i = 0; i < maxPayloadSize; ++i) {
                        sstr << hex << (int) payload[i] << " ";
                    }
                    logMsg(LOG_ERR,
                           "pipeNum is " + to_string((int) pipeNum)
                           + ", which should never happen!  Payload contents:  " + sstr.str());
            }
        }

        // There are no payloads to process, so give other threads a chance to run.
        this_thread::yield();
    }
}


int main(int argc, char** argv)
{
    // Read configuration from the JSON file specified on the command line.
    if (argc != 2) {
        cout << "Usage:  " << argv[0] << " <configFileName>" << endl;
        return 2;
    }
    string configFileName(argv[1]);
    if (!readConfig(configFileName)) {
        return(EXIT_FAILURE);
    }

    if (acquireProcessLock(lockFilePath) < 0) {
        exit(EXIT_FAILURE);
    }

    logMsg(LOG_INFO, "---------- widgetRcvr starting ----------");

    if (!openUdpPorts()) {
        exit(EXIT_FAILURE);
    }

    if (!configureRadio()) {
        exit(EXIT_FAILURE);
    }

    radio.startListening();

    runLoop();
    
    return 0;
}

