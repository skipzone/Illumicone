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

#include <arpa/inet.h>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <map>
//#include <netinet/in.h>
#include <sstream>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <string.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <thread>
#include <time.h>
#include <unistd.h>
#include <vector>

#include "ConfigReader.h"
#include "illumiconeTypes.h"
#include "Pattern.h"
#include "RgbVerticalPattern.h"
#include "AnnoyingFlashingPattern.h"
#include "SparklePattern.h"
#include "HorizontalStripePattern.h"
//#include "RainbowExplosionPattern.h"
#include "Widget.h"
#include "WidgetChannel.h"
#include "WidgetFactory.h"

using namespace std;


constexpr char lockFilePath[] = "/tmp/PatternController.lock";

static ConfigReader config;
static unsigned int numberOfStrings;
static unsigned int numberOfPixelsPerString;

static struct sockaddr_in server;
static int sock;
static uint8_t* opcBuffer;      // points to the buffer used for sending messages to the OPC server
static size_t opcBufferSize;
static uint8_t* opcData;        // points to the data portion of opcBuffer

static map<WidgetId, Widget*> widgets;

static AnnoyingFlashingPattern annoyingFlashingPattern;
static RgbVerticalPattern rgbVerticalPattern;
static SparklePattern sparklePattern;
static HorizontalStripePattern horizontalStripePattern;
//static RainbowExplosionPattern rainbowExplosionPattern;

static map<Pattern*, bool> patternIsOk;


const string getTimestamp()
{
    using namespace std::chrono;

    milliseconds epochMs = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
    int ms = epochMs.count() % 1000;
    time_t now = epochMs.count() / 1000;

    struct tm tmStruct = *localtime(&now);
    char buf[20];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tmStruct);

    stringstream sstr;
    sstr << buf << "." << setfill('0') << setw(3) << ms << ":  ";

    string str = sstr.str();
    return str;
}


bool setUpOpcServerConnection(const string& opcServerIpAddress)
{
    sock = socket(AF_INET, SOCK_STREAM, 0);
    server.sin_addr.s_addr = inet_addr(opcServerIpAddress.c_str());
    server.sin_family = AF_INET;
    server.sin_port = htons(7890);

    if (connect(sock, (struct sockaddr *) &server, sizeof(server)) < 0) {
        cout << "SOMETHING'S FUCKY: couldn't connect to opc-server!" << endl;
        while (1);
    }

    return true;
}


void dumpOpcBuffer()
{
    cout << "opcBuffer[0]: " << unsigned(opcBuffer[0]) << endl;
    cout << "opcBuffer[1]: " << unsigned(opcBuffer[1]) << endl;
    cout << "opcBuffer[2]: " << unsigned(opcBuffer[2]) << endl;
    cout << "opcBuffer[3]: " << unsigned(opcBuffer[3]) << endl;

    // Print just data for the first two strings.
    for (size_t i = 0; i < 2 * numberOfPixelsPerString * 3; i++) {
        cout << unsigned(opcData[i]) << " " << unsigned(opcData[i+1]) << " " << unsigned(opcData[i+2]) << endl;
    }
}


void sendOpcMessage(std::vector<std::vector<opc_pixel_t>> &finalFrame)
{
    for (unsigned int col = 0; col < numberOfStrings; col++) {
        unsigned int colOffset = col * numberOfPixelsPerString * 3;
        for (unsigned int row = 0; row < numberOfPixelsPerString; row++) {
            unsigned int pixelOffset = colOffset + row * 3;
            opcData[pixelOffset] = finalFrame[col][row].r;
            opcData[pixelOffset + 1] = finalFrame[col][row].g;
            opcData[pixelOffset + 2] = finalFrame[col][row].b;
        }
    }

    //dumpOpcBuffer(opcBuffer);

    // send to OPC server over network connection
    send(sock, opcBuffer, opcBufferSize, 0);
}


void zeroFrame(std::vector<std::vector<opc_pixel_t>> &finalFrame)
{
    int col;
    int row;

    for (col = 0; col < numberOfStrings; col++) {
        for (row = 0; row < numberOfPixelsPerString; row++) {
            finalFrame[col][row].r = 0;
            finalFrame[col][row].g = 0;
            finalFrame[col][row].b = 0;
        }
    }
}


/*
 * Build the OPC packet based on the values in the pixelArray for a pattern, and
 * its priority.
 *
 * This will do some sort of math based on its parameters to determine how to update
 * the final array.  The final array will be stored separately as a 2d opc_pixel_t
 * vector, probably globally like opcBuffer.
 *
 * We might have two to provide for double buffering of some sort.
 *
 * Ideally this would be called for each pattern that has updates before sending
 * the final packet over the network, like:
 *      buildPacket(finalFrame, rgbPattern.pixelArray, rgbPattern.priority);
 *      buildPacket(finalFrame, annoyingFlashingPattern.pixelArray, annoyingFlashingPattern.priority);
 *      ..
 *      ..
 *
 *      sendPacket(finalFrame);
 *
 * Call this in reverse priority for best results, i.e. call with priority 3 pattern, then with
 * 2, then 1, then 0.
 */
bool buildFrame(
        std::vector<std::vector<opc_pixel_t>> &finalFrame,
        std::vector<std::vector<opc_pixel_t>> &pixelArray,
        int priority)
{
    int col;
    int row;
    //bool foundNonZeroValue = false;

    switch (priority) {
        case 0:
            //
            // AnnoyingFlashingPattern
            //
            for (col = 0; col < numberOfStrings; col++) {
                for (row = 0; row < numberOfPixelsPerString; row++) {
                    if (pixelArray[col][row].r != 0 && pixelArray[col][row].g != 0 && pixelArray[col][row].b != 0) {
                        finalFrame[col][row] = pixelArray[col][row];
                    }
                }
            }
            break;

        case 1:
            //cout << "build frame plunger" << endl;
            //
            // RainbowExplosionPattern
            //
            for (col = 0; col < numberOfStrings; col++) {
                for (row = 0; row < numberOfPixelsPerString; row++) {
                    if (pixelArray[col][row].r != 0 || pixelArray[col][row].g != 0 || pixelArray[col][row].b != 0) {
                        finalFrame[col][row] = pixelArray[col][row]; 
                    }
                }
            }
            break;

        case 2:
            //cout << "build frame rgb" << endl;
            //
            // RgbVerticalPattern
            //
            for (col = 0; col < numberOfStrings; col++) {
                for (row = 0; row < numberOfPixelsPerString; row++) {
                    // only update the value of the final frame if the pixel
                    // contains non-zero values (is on)
                    if (pixelArray[col][row].r != 0 || pixelArray[col][row].g != 0 || pixelArray[col][row].b != 0) {
                        //foundNonZeroValue = true;
                        finalFrame[col][row] = pixelArray[col][row];
                    }
                }
            }
            //if (foundNonZeroValue) {
            //    cout << "found non-zero pixel value for RgbVerticalPattern" << endl;
            //}
            break;
 
        case 3:
            //cout << "build frame fourplay" << endl;
            //
            // HorizontalStripePattern
            //
            for (col = 0; col < numberOfStrings; col++) {
                for (row = 0; row < numberOfPixelsPerString; row++) {
                    if (pixelArray[col][row].r != 0 || pixelArray[col][row].g != 0 || pixelArray[col][row].b != 0) {
                        finalFrame[col][row] = pixelArray[col][row];
                    }
                }
            }
            break;

         case 4:
            //
            // SparklePattern
            //
            for (col = 0; col < numberOfStrings; col++) {
                for (row = 0; row < numberOfPixelsPerString; row++) {
                    if (pixelArray[col][row].r != 0 || pixelArray[col][row].g != 0 || pixelArray[col][row].b != 0) {
                        finalFrame[col][row] = pixelArray[col][row];
                    }
                }
            }
            break;

          
        case 5:
            //cout << "build frame quad" << endl;
            //
            // QuadSlicePattern
            //
            for (col = 0; col < numberOfStrings; col++) {
                for (row = 0; row < numberOfPixelsPerString; row++) {
                    // only update the value of the final frame if the pixel
                    // contains non-zero values (is on)
                    if (pixelArray[col][row].r != 0 || pixelArray[col][row].g != 0 || pixelArray[col][row].b != 0) {
                        finalFrame[col][row] = pixelArray[col][row];
                    }
                }
            }
            break;

        default:
            cout << "SOMETHING'S FUCKY : no case for priority " << priority << endl;
    }

    return true;
}

//
// light each string at the bottom for visibility
//
void finalizeFrame(std::vector<std::vector<opc_pixel_t>> &finalFrame)
{
    for (auto&& stringPixels : finalFrame) {
        stringPixels[numberOfPixelsPerString - 1].r = 255;
        stringPixels[numberOfPixelsPerString - 1].g = 0;
        stringPixels[numberOfPixelsPerString - 1].b = 255;
    }
}


bool readConfig(const string& configFileName)
{
    if (!config.readConfigurationFile(configFileName)) {
        return false;
    }

    numberOfStrings = config.getNumberOfStrings();
    numberOfPixelsPerString = config.getNumberOfPixelsPerString();
    cout << "numberOfStrings = " << numberOfStrings << endl;
    cout << "numberOfPixelsPerString = " << numberOfPixelsPerString << endl;

    return true;
}


int acquireProcessLock()
{
    int fd = open(lockFilePath, O_CREAT, S_IRUSR | S_IWUSR);
    if (fd >= 0) {
        if (flock(fd, LOCK_EX | LOCK_NB) == 0) {
            return fd;
        }
        else {
            if (errno == EWOULDBLOCK) {
                close(fd);
                fprintf(stderr, "Another process has locked %s.\n", lockFilePath);
                return -1;
            }
            else {
                close(fd);
                fprintf(stderr, "Unable to lock %s.  Error %d:  %s\n", lockFilePath, errno, strerror(errno));
                return -1;
            }
        }
    }
    else {
        fprintf(stderr, "Unable to create or open %s.  Error %d:  %s\n", lockFilePath, errno, strerror(errno));
        return -1;
    }
}


bool initOpcBuffer()
{
    // Allocate the buffer that will be used to build and send the Open Pixel Control (OPC) messages.
    opcBufferSize = numberOfStrings * numberOfPixelsPerString * 3 + 4;
    opcBuffer = new uint8_t[opcBufferSize];
    if (opcBuffer == nullptr) {
        cerr << "Unable to allocate an OPC buffer of size " << opcBufferSize << endl;
        return false;
    }

    // Set up the header now because it won't change.
    opcBuffer[0] = 0;   // channel
    opcBuffer[1] = 0;   // command
    opcBuffer[2] = numberOfStrings * numberOfPixelsPerString * 3 / 256; // data length high byte
    opcBuffer[3] = numberOfStrings * numberOfPixelsPerString * 3 % 256; // data length low byte

    // The data portion of the OPC message starts just after the header.
    opcData = &opcBuffer[4];

    return true;
}


void initWidgets()
{
    cout << "Initializing widgets..." << endl;

    // TODO 6/12/2017 ross:  Move widget config access to ConfigReader.

    for (auto& widgetConfig : config.getJsonObject()["widgets"].array_items()) {
        string widgetName = widgetConfig["name"].string_value();
        if (widgetName.empty()) {
            cerr << "Widget configuration has no name:  " << widgetConfig.dump() << endl;
            continue;
        }
        if (!widgetConfig["enabled"].bool_value()) {
            cout << widgetName << " is disabled." << endl;
            continue;
        }
        WidgetId widgetId = stringToWidgetId(widgetName);
        if (widgetId == WidgetId::invalid) {
            cerr << "Widget configuration has invalid name:  " << widgetConfig.dump() << endl;
            continue;
        }
        if (widgets.find(widgetId) != widgets.end()) {
            cerr << widgetName << " appears multiple times.  This configuration ignored:  " << widgetConfig.dump() << endl;
            continue;
        }
        Widget* newWidget = widgetFactory(widgetId);
        if (newWidget == nullptr) {
            cerr << "Unable to instantiate Widget object for " << widgetName << endl;
            continue;
        }
        if (!newWidget->init(config)) {
            cerr << "Unable to initialize Widget object for " << widgetName << endl;
            continue;
        }
        cout << widgetName << " initialized." << endl;
        widgets[widgetId] = newWidget;
    }
}


void initPatterns()
{
    cout << "Initializing patterns..." << endl;

    // TODO:  Get priorities from config file.

    if (annoyingFlashingPattern.initPattern(config, widgets, 0)) {
        patternIsOk[&annoyingFlashingPattern] = true;
        cout << "annoyingFlashingPattern ok" << endl;
    }
    else {
        cout << "annoyingFlashingPattern initialization failed." << endl;
    }

//    rainbowExplosionPattern.initPattern(numberOfStrings, numberOfPixelsPerString, priorities[1]);
//    rainbowExplosionPattern.initWidgets(1, numChannels[1]);
//    printPatternConfig(rainbowExplosionPattern);

    if (rgbVerticalPattern.initPattern(config, widgets, 2)) {
        patternIsOk[&rgbVerticalPattern] = true;
        cout << "rgbVerticalPattern ok" << endl;
    }
    else {
        cout << "rgbVerticalPattern initialization failed." << endl;
    }

    if (horizontalStripePattern.initPattern(config, widgets, 3)) {
        patternIsOk[&horizontalStripePattern] = true;
        cout << "horizontalStripePattern ok" << endl;
    }
    else {
        cout << "horizontalStripePattern initialization failed." << endl;
    }

    if (sparklePattern.initPattern(config, widgets, 4)) {
        patternIsOk[&sparklePattern] = true;
        cout << "sparklePattern ok" << endl;
    }
    else {
        cout << "sparklePattern initialization failed." << endl;
    }
}


void moveWidgetData()
{
    for (auto&& widget : widgets) {
        widget.second->moveData();
    }
}


void doPatterns()
{
    static bool doIdlePattern;
    static time_t timeWentIdle;

    if (patternIsOk.find(&annoyingFlashingPattern) != patternIsOk.end()) {
        annoyingFlashingPattern.update();
    }
    if (patternIsOk.find(&rgbVerticalPattern) != patternIsOk.end()) {
        rgbVerticalPattern.update();
    }
    if (patternIsOk.find(&horizontalStripePattern) != patternIsOk.end()) {
        horizontalStripePattern.update();
    }
    if (patternIsOk.find(&sparklePattern) != patternIsOk.end()) {
        sparklePattern.update();
    }
//    rainbowExplosionPattern.update();

    bool anyPatternIsActive = false;

    vector<vector<opc_pixel_t>> finalFrame1;
    finalFrame1.resize(numberOfStrings, std::vector<opc_pixel_t>(numberOfPixelsPerString));
    zeroFrame(finalFrame1);

    if (sparklePattern.isActive) {
        anyPatternIsActive = true;
        //cout << "sparkle active" << endl;
        buildFrame(finalFrame1, sparklePattern.pixelArray, sparklePattern.priority);
    }
   
    if (horizontalStripePattern.isActive) {
        anyPatternIsActive = true;
        //cout << "horizontalStripe active" << endl;
        buildFrame(finalFrame1, horizontalStripePattern.pixelArray, horizontalStripePattern.priority);
    }

    if (rgbVerticalPattern.isActive) {
        anyPatternIsActive = true;
        //cout << "rgbVertical active" << endl;
        buildFrame(finalFrame1, rgbVerticalPattern.pixelArray, rgbVerticalPattern.priority);
    }

//    if (rainbowExplosionPattern.isActive) {
//        anyPatternIsActive = true;
//        buildFrame(finalFrame1, rainbowExplosionPattern.pixelArray, rainbowExplosionPattern.priority);
//    }

    if (annoyingFlashingPattern.isActive) {
        anyPatternIsActive = true;
        //cout << "annoyingFlashing active" << endl;
        buildFrame(finalFrame1, annoyingFlashingPattern.pixelArray, annoyingFlashingPattern.priority);
    }

    if (!anyPatternIsActive) {
        //cout << "no patterns are active; finalizing finalFrame1" << endl;
        finalizeFrame(finalFrame1);
    }

    if (!anyPatternIsActive) {
        if (timeWentIdle == 0) {
            time(&timeWentIdle);
        }
        else {
            time_t now;
            time(&now);
            if (!doIdlePattern && now - timeWentIdle > 3) {
                doIdlePattern = true;
            }
        }
    }
    else {
        doIdlePattern = false;
        timeWentIdle = 0;
    }

    if (!doIdlePattern) {
        sendOpcMessage(finalFrame1);
    }
}


int main(int argc, char **argv)
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

    // Make sure this is the only instance running.
    if (acquireProcessLock() < 0) {
        return(EXIT_FAILURE);
    }

    cout << getTimestamp() << "---------- PatternController  starting ----------" << endl;

    if (!initOpcBuffer()) {
        return(EXIT_FAILURE);
    }

    // open socket, connect with opc-server
    setUpOpcServerConnection(config.getOpcServerIpAddress());

    initWidgets();
    initPatterns();
    cout << "Pattern initialization done.  Start moving shit!" << endl;

    while (true) {
        moveWidgetData();
        doPatterns();
        // TODO 6/25/2017 ross:  Use an actual interval.  Set it in the JSON config.
        usleep(10000);
    }

    // We should never get here, but if we do, something went wrong.
    return EXIT_FAILURE;
}

