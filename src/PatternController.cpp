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
#include "log.h"
#include "Pattern.h"
#include "RgbVerticalPattern.h"
#include "AnnoyingFlashingPattern.h"
#include "SparklePattern.h"
#include "HorizontalStripePattern.h"
//#include "RainbowExplosionPattern.h"
#include "FillAndBurstPattern.h"
#include "ParticlesPattern.h"
#include "Widget.h"
#include "WidgetChannel.h"
#include "WidgetFactory.h"

using namespace std;


constexpr char lockFilePath[] = "/tmp/PatternController.lock";

static ConfigReader config;
static unsigned int numberOfStrings;
static unsigned int numberOfPixelsPerString;
static vector<SchedulePeriod> shutoffPeriods;
static vector<SchedulePeriod> quiescentPeriods;

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
static FillAndBurstPattern fillAndBurstPattern;
static ParticlesPattern particlesPattern;

static map<Pattern*, bool> patternIsOk;


bool setUpOpcServerConnection(const string& opcServerIpAddress)
{
    sock = socket(AF_INET, SOCK_STREAM, 0);
    server.sin_addr.s_addr = inet_addr(opcServerIpAddress.c_str());
    server.sin_family = AF_INET;
    server.sin_port = htons(7890);

    if (connect(sock, (struct sockaddr *) &server, sizeof(server)) < 0) {
        logMsg(LOG_ERR, "SOMETHING'S FUCKY: couldn't connect to opc-server!");
        return false;
    }

    return true;
}


void dumpOpcBuffer()
{
    logMsg(LOG_DEBUG, "opcBuffer[0]: " + to_string(unsigned(opcBuffer[0])));
    logMsg(LOG_DEBUG, "opcBuffer[1]: " + to_string(unsigned(opcBuffer[1])));
    logMsg(LOG_DEBUG, "opcBuffer[2]: " + to_string(unsigned(opcBuffer[2])));
    logMsg(LOG_DEBUG, "opcBuffer[3]: " + to_string(unsigned(opcBuffer[3])));

    // Print just data for the first two strings.
    for (size_t i = 0; i < 2 * numberOfPixelsPerString * 3; i++) {
        logMsg(LOG_DEBUG, to_string(unsigned(opcData[i]))
                + " " + to_string(unsigned(opcData[i+1]))
                + " " + to_string(unsigned(opcData[i+2])));
    }
}


void sendOpcMessage(vector<vector<opc_pixel_t>> &finalFrame)
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


void zeroFrame(vector<vector<opc_pixel_t>> &finalFrame)
{
    for (unsigned int col = 0; col < numberOfStrings; col++) {
        for (unsigned int row = 0; row < numberOfPixelsPerString; row++) {
            finalFrame[col][row].r = 0;
            finalFrame[col][row].g = 0;
            finalFrame[col][row].b = 0;
        }
    }
}


void turnOffAllPixels()
{
    vector<vector<opc_pixel_t>> finalFrame;
    finalFrame.resize(numberOfStrings, vector<opc_pixel_t>(numberOfPixelsPerString));
    zeroFrame(finalFrame);
    sendOpcMessage(finalFrame);
}


void setAllPixelsToQuiescentColor()
{
    vector<vector<opc_pixel_t>> finalFrame;
    finalFrame.resize(numberOfStrings, vector<opc_pixel_t>(numberOfPixelsPerString));
    for (unsigned int col = 0; col < numberOfStrings; col++) {
        for (unsigned int row = 0; row < numberOfPixelsPerString; row++) {
            finalFrame[col][row].r = 0;
            finalFrame[col][row].g = 0;
            finalFrame[col][row].b = 64;
        }
    }
    sendOpcMessage(finalFrame);
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
        vector<vector<opc_pixel_t>> &finalFrame,
        vector<vector<opc_pixel_t>> &pixelArray,
        int priority)
{
    // TODO 7/15/2017 ross:  All these cases end up doing the same thing.  We need a different approach.

    switch (priority) {
        case 0:
            // AnnoyingFlashingPattern
            for (unsigned int col = 0; col < numberOfStrings; col++) {
                for (unsigned int row = 0; row < numberOfPixelsPerString; row++) {
                    if (pixelArray[col][row].r != 0 && pixelArray[col][row].g != 0 && pixelArray[col][row].b != 0) {
                        finalFrame[col][row] = pixelArray[col][row];
                    }
                }
            }
            break;

        case 1:
            // FillAndBurstPattern, bursting
            for (unsigned int col = 0; col < numberOfStrings; col++) {
                for (unsigned int row = 0; row < numberOfPixelsPerString; row++) {
                    if (pixelArray[col][row].r != 0 || pixelArray[col][row].g != 0 || pixelArray[col][row].b != 0) {
                        finalFrame[col][row] = pixelArray[col][row]; 
                    }
                }
            }
            break;

        case 2:
            // RgbVerticalPattern
            for (unsigned int col = 0; col < numberOfStrings; col++) {
                for (unsigned int row = 0; row < numberOfPixelsPerString; row++) {
                    // only update the value of the final frame if the pixel
                    // contains non-zero values (is on)
                    if (pixelArray[col][row].r != 0 || pixelArray[col][row].g != 0 || pixelArray[col][row].b != 0) {
                        finalFrame[col][row] = pixelArray[col][row];
                    }
                }
            }
            break;
 
        case 3:
            // HorizontalStripePattern
            for (unsigned int col = 0; col < numberOfStrings; col++) {
                for (unsigned int row = 0; row < numberOfPixelsPerString; row++) {
                    if (pixelArray[col][row].r != 0 || pixelArray[col][row].g != 0 || pixelArray[col][row].b != 0) {
                        finalFrame[col][row] = pixelArray[col][row];
                    }
                }
            }
            break;

        case 4:
            // ParticlesPattern
            for (unsigned int col = 0; col < numberOfStrings; col++) {
                for (unsigned int row = 0; row < numberOfPixelsPerString; row++) {
                    if (pixelArray[col][row].r != 0 || pixelArray[col][row].g != 0 || pixelArray[col][row].b != 0) {
                        finalFrame[col][row] = pixelArray[col][row]; 
                    }
                }
            }
            break;

         case 5:
            // SparklePattern
            for (unsigned int col = 0; col < numberOfStrings; col++) {
                for (unsigned int row = 0; row < numberOfPixelsPerString; row++) {
                    if (pixelArray[col][row].r != 0 || pixelArray[col][row].g != 0 || pixelArray[col][row].b != 0) {
                        finalFrame[col][row] = pixelArray[col][row];
                    }
                }
            }
            break;
          
        case 6:
            // FillAndBurstPattern, pressurizing
            for (unsigned int col = 0; col < numberOfStrings; col++) {
                for (unsigned int row = 0; row < numberOfPixelsPerString; row++) {
                    if (pixelArray[col][row].r != 0 || pixelArray[col][row].g != 0 || pixelArray[col][row].b != 0) {
                        finalFrame[col][row] = pixelArray[col][row]; 
                    }
                }
            }
            break;

        default:
            logMsg(LOG_ERR, "SOMETHING'S FUCKY : no case for priority " + to_string(priority));
    }

    return true;
}

//
// light each string at the bottom for visibility
//
void finalizeFrame(vector<vector<opc_pixel_t>> &finalFrame)
{
    for (auto&& stringPixels : finalFrame) {
        stringPixels[numberOfPixelsPerString - 1].r = 255;
        stringPixels[numberOfPixelsPerString - 1].g = 0;
        stringPixels[numberOfPixelsPerString - 1].b = 255;
    }
}


void printSchedulePeriods(const std::string& scheduleDescription, const vector<SchedulePeriod>& schedulePeriods)
{
    logMsg(LOG_INFO, scheduleDescription + ":");

    for (auto&& schedulePeriod : schedulePeriods) {
        string msg;
        int msgPriority = LOG_INFO;

        struct tm tmStartTime;
        struct tm tmEndTime;
        localtime_r(&schedulePeriod.startTime, &tmStartTime);
        localtime_r(&schedulePeriod.endTime, &tmEndTime);
        char startTimeBuf[20];
        char endTimeBuf[20];
        msg = "    " + schedulePeriod.description + ":  ";
        string dateTimeFormat;
        if (schedulePeriod.isDaily) {
            dateTimeFormat = "%H:%M:%S";
            msg += "daily, ";
        }
        else {
            dateTimeFormat = "%Y-%m-%d %H:%M:%S";
        }
        if (strftime(startTimeBuf, sizeof(startTimeBuf), dateTimeFormat.c_str(), &tmStartTime) != 0
            && strftime(endTimeBuf, sizeof(endTimeBuf), dateTimeFormat.c_str(), &tmEndTime) != 0)
        {
            msg += string(startTimeBuf) + " - " + string(endTimeBuf);
        }
        else
        {
            msg += "is shit!";
            msgPriority = LOG_ERR;
        }

        logMsg(msgPriority, msg);
    }
}


bool readConfig(const string& configFileName)
{
    if (!config.readConfigurationFile(configFileName)) {
        return false;
    }

    numberOfStrings = config.getNumberOfStrings();
    logMsg(LOG_INFO, "numberOfStrings = " + to_string(numberOfStrings));
    numberOfPixelsPerString = config.getNumberOfPixelsPerString();
    logMsg(LOG_INFO, "numberOfPixelsPerString = " + to_string(numberOfPixelsPerString));

    if (config.getSchedulePeriods("shutoffPeriods", shutoffPeriods)
        || config.getSchedulePeriods("quiescentPeriods", quiescentPeriods))
    {
        return false;
    }
    printSchedulePeriods("Shutoff periods", shutoffPeriods);
    printSchedulePeriods("Quiescent periods", quiescentPeriods);

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
        logMsg(LOG_ERR, "Unable to allocate an OPC buffer of size " + to_string(opcBufferSize));
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
    logMsg(LOG_INFO, "Initializing widgets...");

    // TODO 6/12/2017 ross:  Move widget config access to ConfigReader.

    for (auto& widgetConfig : config.getJsonObject()["widgets"].array_items()) {
        string widgetName = widgetConfig["name"].string_value();
        if (widgetName.empty()) {
            logMsg(LOG_ERR, "Widget configuration has no name:  " + widgetConfig.dump());
            continue;
        }
        if (!widgetConfig["enabled"].bool_value()) {
            logMsg(LOG_INFO, widgetName + " is disabled.");
            continue;
        }
        WidgetId widgetId = stringToWidgetId(widgetName);
        if (widgetId == WidgetId::invalid) {
            logMsg(LOG_ERR, "Widget configuration has invalid name:  " + widgetConfig.dump());
            continue;
        }
        if (widgets.find(widgetId) != widgets.end()) {
            logMsg(LOG_ERR, widgetName + " appears multiple times.  This configuration ignored:  " + widgetConfig.dump());
            continue;
        }
        Widget* newWidget = widgetFactory(widgetId);
        if (newWidget == nullptr) {
            logMsg(LOG_ERR, "Unable to instantiate Widget object for " + widgetName);
            continue;
        }
        if (!newWidget->init(config)) {
            logMsg(LOG_ERR, "Unable to initialize Widget object for " + widgetName);
            continue;
        }
        logMsg(LOG_INFO, widgetName + " initialized.");
        widgets[widgetId] = newWidget;
    }
}


void initPatterns()
{
    logMsg(LOG_INFO, "Initializing patterns...");

    // TODO:  Get priorities from config file.

    if (annoyingFlashingPattern.initPattern(config, widgets, 0)) {
        patternIsOk[&annoyingFlashingPattern] = true;
        logMsg(LOG_INFO, "annoyingFlashingPattern ok");
    }
    else {
        logMsg(LOG_ERR, "annoyingFlashingPattern initialization failed.");
    }

    // FillAndBurstPattern will change its priority based on its state--6 while pressurizing, 1 while bursting.
    if (fillAndBurstPattern.initPattern(config, widgets, 1)) {
        patternIsOk[&fillAndBurstPattern] = true;
        logMsg(LOG_INFO, "fillAndBurstPattern ok");
    }
    else {
        logMsg(LOG_ERR, "fillAndBurstPattern initialization failed.");
    }

    if (rgbVerticalPattern.initPattern(config, widgets, 2)) {
        patternIsOk[&rgbVerticalPattern] = true;
        logMsg(LOG_INFO, "rgbVerticalPattern ok");
    }
    else {
        logMsg(LOG_ERR, "rgbVerticalPattern initialization failed.");
    }

    if (horizontalStripePattern.initPattern(config, widgets, 3)) {
        patternIsOk[&horizontalStripePattern] = true;
        logMsg(LOG_INFO, "horizontalStripePattern ok");
    }
    else {
        logMsg(LOG_ERR, "horizontalStripePattern initialization failed.");
    }

    if (particlesPattern.initPattern(config, widgets, 4)) {
        patternIsOk[&particlesPattern] = true;
        logMsg(LOG_INFO, "particlesPattern ok");
    }
    else {
        logMsg(LOG_ERR, "particlesPattern initialization failed.");
    }

    if (sparklePattern.initPattern(config, widgets, 5)) {
        patternIsOk[&sparklePattern] = true;
        logMsg(LOG_INFO, "sparklePattern ok");
    }
    else {
        logMsg(LOG_ERR, "sparklePattern initialization failed.");
    }
}


bool timeIsInPeriod(time_t now, const vector<SchedulePeriod>& schedulePeriods, string& periodDescription)
{
    // For daily events, we need a tm structure containing the current
    // time so that we can set a daily event's date to today.
    struct tm tmNowTime = *localtime(&now);

    for (auto&& schedulePeriod : schedulePeriods) {
        if (schedulePeriod.isDaily) {
            // Modify the start and end times so that they occur today.
            struct tm tmStartTimeToday = *localtime(&schedulePeriod.startTime);
            struct tm tmEndTimeToday = *localtime(&schedulePeriod.endTime);
            tmStartTimeToday.tm_year = tmEndTimeToday.tm_year = tmNowTime.tm_year;
            tmStartTimeToday.tm_mon = tmEndTimeToday.tm_mon = tmNowTime.tm_mon;
            tmStartTimeToday.tm_mday = tmEndTimeToday.tm_mday = tmNowTime.tm_mday;
            tmStartTimeToday.tm_isdst = tmEndTimeToday.tm_isdst = tmNowTime.tm_isdst;
            time_t startTimeToday = mktime(&tmStartTimeToday);
            time_t endTimeToday = mktime(&tmEndTimeToday);
            //cout << "desc=" << schedulePeriod.description << ", now=" << now << ", startTime="
            //    << startTimeToday << ", endTime=" << endTimeToday << endl;
            // Periods that span midnight have an end time that is numerically less
            // than the start time (which actually occurs on the previous day).
            if (endTimeToday < startTimeToday) {
                if (now >= startTimeToday || now <= endTimeToday) {
                    periodDescription = schedulePeriod.description;
                    return true;
                }
            }
            else {
                if (now >= startTimeToday && now <= endTimeToday) {
                    periodDescription = schedulePeriod.description;
                    return true;
                }
            }
        }
        else {
            // This is a one-time event.
            //cout << "desc=" << schedulePeriod.description << ", now=" << now << ", startTime="
            //    << schedulePeriod.startTime << ", endTime=" << schedulePeriod.endTime << endl;
            if (now >= schedulePeriod.startTime && now <= schedulePeriod.endTime) {
                periodDescription = schedulePeriod.description;
                return true;
            }
        }
    }

    return false;
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
    if (patternIsOk.find(&fillAndBurstPattern) != patternIsOk.end()) {
        fillAndBurstPattern.update();
    }
    if (patternIsOk.find(&rgbVerticalPattern) != patternIsOk.end()) {
        rgbVerticalPattern.update();
    }
    if (patternIsOk.find(&horizontalStripePattern) != patternIsOk.end()) {
        horizontalStripePattern.update();
    }
    if (patternIsOk.find(&particlesPattern) != patternIsOk.end()) {
        particlesPattern.update();
    }
    if (patternIsOk.find(&sparklePattern) != patternIsOk.end()) {
        sparklePattern.update();
    }

    bool anyPatternIsActive = false;

    vector<vector<opc_pixel_t>> finalFrame1;
    finalFrame1.resize(numberOfStrings, vector<opc_pixel_t>(numberOfPixelsPerString));
    zeroFrame(finalFrame1);

    if (fillAndBurstPattern.isActive && fillAndBurstPattern.priority == 6) {
        anyPatternIsActive = true;
        //logMsg(LOG_DEBUG, "fillAndBurst active while pressurizing.");
        buildFrame(finalFrame1, fillAndBurstPattern.pixelArray, fillAndBurstPattern.priority);
    }

    if (sparklePattern.isActive) {
        anyPatternIsActive = true;
        buildFrame(finalFrame1, sparklePattern.pixelArray, sparklePattern.priority);
    }

    if (particlesPattern.isActive) {
        anyPatternIsActive = true;
        buildFrame(finalFrame1, particlesPattern.pixelArray, particlesPattern.priority);
    }
   
    if (horizontalStripePattern.isActive) {
        anyPatternIsActive = true;
        buildFrame(finalFrame1, horizontalStripePattern.pixelArray, horizontalStripePattern.priority);
    }

    if (rgbVerticalPattern.isActive) {
        anyPatternIsActive = true;
        buildFrame(finalFrame1, rgbVerticalPattern.pixelArray, rgbVerticalPattern.priority);
    }

    if (fillAndBurstPattern.isActive && fillAndBurstPattern.priority == 1) {
        anyPatternIsActive = true;
        //logMsg(LOG_DEBUG, "fillAndBurst active while bursting.");
        buildFrame(finalFrame1, fillAndBurstPattern.pixelArray, fillAndBurstPattern.priority);
    }

    if (annoyingFlashingPattern.isActive) {
        anyPatternIsActive = true;
        buildFrame(finalFrame1, annoyingFlashingPattern.pixelArray, annoyingFlashingPattern.priority);
    }

    if (!anyPatternIsActive) {
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

    logMsg(LOG_INFO, "---------- PatternController  starting ----------");

    if (!initOpcBuffer()) {
        return(EXIT_FAILURE);
    }

    // open socket, connect with opc-server
    if (!setUpOpcServerConnection(config.getOpcServerIpAddress())) {
        return(EXIT_FAILURE);
    }

    initWidgets();
    initPatterns();
    logMsg(LOG_INFO, "Pattern initialization done.  Start moving shit!");


    time_t lastPeriodCheckTime = 0;
    bool inPeriod = false;
    string lastPeriodDesc = "";

    // ----- run loop -----
    while (true) {

        moveWidgetData();

        // Once per second, check if we're in a schedule period.
        time_t now;
        time(&now);
        if (now > lastPeriodCheckTime) {
            lastPeriodCheckTime = now;

            // TODO:  Log appropriate messages at both the start and end of each period.

            string periodDesc;
            if (timeIsInPeriod(now, shutoffPeriods, periodDesc)) {
                inPeriod = true;
                if (periodDesc != lastPeriodDesc) {
                    lastPeriodDesc = periodDesc;
                    logMsg(LOG_INFO, "In \"" + periodDesc + "\" shutoff period.");
                }
                turnOffAllPixels();
            }
            else if (timeIsInPeriod(now, quiescentPeriods, periodDesc)) {
                inPeriod = true;
                if (periodDesc != lastPeriodDesc) {
                    lastPeriodDesc = periodDesc;
                    logMsg(LOG_INFO, "In \"" + periodDesc + "\" quiescent period.");
                }
                setAllPixelsToQuiescentColor();
            }
            else {
                inPeriod = false;
            }
        }

        if (!inPeriod) {
            doPatterns();
        }

        // TODO 6/25/2017 ross:  Use an actual interval.  Set it in the JSON config.
        usleep(5000);
    }

    // We should never get here, but if we do, something went wrong.
    return EXIT_FAILURE;
}

