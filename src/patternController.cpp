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

#include <algorithm>
#include <arpa/inet.h>
#include <climits>
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
#include "hsv2rgb.h"
#include "illumiconeTypes.h"
#include "illumiconePixelUtility.h"
//#include "illumiconeUtility.h"
#include "lib8tion.h"
#include "log.h"
#include "Pattern.h"
#include "patternFactory.h"
#include "pixeltypes.h"
#include "Widget.h"
#include "WidgetChannel.h"
#include "widgetFactory.h"

using namespace std;


// TODO 7/31/2017 ross:  Get this from config.
constexpr char lockFilePath[] = "/tmp/patternController.lock";

enum class PatternBlendMethod {
    overlay,
    rgbAdd,
    rgbBlend,
    hsvBlend,
    hsvHueBlend
};

struct PatternState {
    Pattern* pattern;
    int priority;
    fract8 amountOfOverlay;
    int wantsDisplay;
};

static ConfigReader config;
static unsigned int numberOfStrings;
static unsigned int numberOfPixelsPerString;
static vector<SchedulePeriod> shutoffPeriods;
static vector<SchedulePeriod> quiescentPeriods;
static string patternBlendMethodStr;
static PatternBlendMethod patternBlendMethod;

static struct sockaddr_in server;
static int sock;
static uint8_t* opcBuffer;      // points to the buffer used for sending messages to the OPC server
static size_t opcBufferSize;
static uint8_t* opcData;        // points to the data portion of opcBuffer

static map<WidgetId, Widget*> widgets;
static vector<PatternState*> patternStates;

static HsvConeStrings hsvFinalFrame;
static RgbConeStrings rgbFinalFrame;


bool setUpOpcServerConnection(const string& opcServerIpAddress)
{
    sock = socket(AF_INET, SOCK_STREAM, 0);
    server.sin_addr.s_addr = inet_addr(opcServerIpAddress.c_str());
    server.sin_family = AF_INET;
    server.sin_port = htons(7890);

    logMsg(LOG_INFO, "Connecting to OPC server at " + opcServerIpAddress + "...");
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


void sendOpcMessage()
{
    for (unsigned int col = 0; col < numberOfStrings; col++) {
        unsigned int colOffset = col * numberOfPixelsPerString * 3;
        for (unsigned int row = 0; row < numberOfPixelsPerString; row++) {
            unsigned int pixelOffset = colOffset + row * 3;
            opcData[pixelOffset] = rgbFinalFrame[col][row].r;
            opcData[pixelOffset + 1] = rgbFinalFrame[col][row].g;
            opcData[pixelOffset + 2] = rgbFinalFrame[col][row].b;
        }
    }

    //dumpOpcBuffer(opcBuffer);

    // send to OPC server over network connection
    send(sock, opcBuffer, opcBufferSize, 0);
}


void turnOffAllPixels()
{
    fillSolid(rgbFinalFrame, RgbPixel::Black);
    sendOpcMessage();
}


void turnOnSafetyLights()
{
    fillSolid(rgbFinalFrame, RgbPixel::Black);
    for (auto&& stringPixels : rgbFinalFrame) {
        // TODO ross 7/22/2017:  get the safety color from config
        stringPixels[numberOfPixelsPerString - 1] = RgbPixel::Magenta;
    }
    sendOpcMessage();
}


void setAllPixelsToQuiescentColor()
{
    fillSolid(rgbFinalFrame, RgbPixel::Navy);
    sendOpcMessage();
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
    numberOfPixelsPerString = config.getNumberOfPixelsPerString();

    if (config.getSchedulePeriods("shutoffPeriods", shutoffPeriods)
        || config.getSchedulePeriods("quiescentPeriods", quiescentPeriods))
    {
        return false;
    }

    patternBlendMethodStr = config.getPatternBlendMethod();
    if (patternBlendMethodStr == "overlay") {
        patternBlendMethod = PatternBlendMethod::overlay;
    }
    else if (patternBlendMethodStr == "rgbAdd") {
        patternBlendMethod = PatternBlendMethod::rgbAdd;
    }
    else if (patternBlendMethodStr == "rgbBlend") {
        patternBlendMethod = PatternBlendMethod::rgbBlend;
    }
    else if (patternBlendMethodStr == "hsvBlend") {
        patternBlendMethod = PatternBlendMethod::hsvBlend;
    }
    else if (patternBlendMethodStr == "hsvHueBlend") {
        patternBlendMethod = PatternBlendMethod::hsvHueBlend;
    }
    else {
        logMsg(LOG_ERR, "patternBlendMethod \"" + patternBlendMethodStr + "\" not recognized.");
        return false;
    }

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

    for (auto& patternConfig : config.getJsonObject()["patterns"].array_items()) {
        string patternName = patternConfig["name"].string_value();
        if (patternName.empty()) {
            logMsg(LOG_ERR, "Pattern configuration has no name:  " + patternConfig.dump());
            continue;
        }
        if (!patternConfig["enabled"].bool_value()) {
            logMsg(LOG_INFO, patternName + " is disabled.");
            continue;
        }
        string patternClassName = patternConfig["patternClassName"].string_value();
        if (patternClassName.empty()) {
            logMsg(LOG_ERR, "Pattern configuration does not have a pattern class name:  " + patternConfig.dump());
            continue;
        }
        Pattern* newPattern = patternFactory(patternClassName, patternName);
        if (newPattern == nullptr) {
            logMsg(LOG_ERR,
                    "Unable to instantiate " + patternClassName + " object for " + patternName
                    + ".  (Is the pattern class name correct?)");
            continue;
        }
        if (!newPattern->init(config, widgets)) {
            logMsg(LOG_ERR, "Unable to initialize Pattern object for " + patternName);
            delete newPattern;
            continue;
        }
        logMsg(LOG_INFO, patternName + " initialized.");

        PatternState* newPatternState = new PatternState;
        newPatternState->pattern = newPattern;
        patternStates.emplace_back(newPatternState);
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


void doPatterns()
{
    static bool doIdlePattern;
    static time_t timeWentIdle;

    // Let all the patterns update their shit.
    bool anyPatternIsActive = false;
    int minPriority = INT_MAX;
    int maxPriority = INT_MIN;
    for (auto&& patternState : patternStates) {
        patternState->wantsDisplay = patternState->pattern->update();
        patternState->priority = patternState->pattern->priority;
        patternState->amountOfOverlay = patternState->pattern->opacity * 255 / 100;
        anyPatternIsActive |= patternState->wantsDisplay;
        minPriority = min(patternState->priority, minPriority);
        maxPriority = max(patternState->priority, maxPriority);
    }

    clearAllPixels(rgbFinalFrame);
    clearAllPixels(hsvFinalFrame);

    if (anyPatternIsActive) {
        bool anyPixelIsOn = false;

        // Layer the patterns into the final frame in reverse priority order
        // (i.e., lowest priority first, highest priority last).  Note that
        // a lower priority value denotes higher priority (0 is highest).
        for (int priority = maxPriority; priority >= minPriority; --priority) {
            for (auto&& patternState : patternStates) {
                if (patternState->priority == priority) {
                    for (unsigned int col = 0; col < numberOfStrings; col++) {
                        for (unsigned int row = 0; row < numberOfPixelsPerString; row++) {

                            // Figure out if the pixel is transparent, and convert
                            // it to the required color model, if necessary.
                            bool pixelIsTransparent;
                            RgbPixel rgbPatPixel;
                            HsvPixel hsvPatPixel;
                            switch (patternBlendMethod) {
                                case PatternBlendMethod::overlay:
                                case PatternBlendMethod::rgbAdd:
                                case PatternBlendMethod::rgbBlend:
                                    if (patternState->pattern->usesHsvModel) {
                                        // TODO:  use hsvTransparent
                                        pixelIsTransparent = patternState->pattern->coneStrings[col][row] == HsvPixel(0, 0, 0);
                                        if (!pixelIsTransparent) {
                                            hsv2rgb_rainbow(patternState->pattern->coneStrings[col][row], rgbPatPixel);
                                        }
                                    }
                                    else {
                                        // TODO:  use rgbTransparent
                                        pixelIsTransparent =
                                            patternState->pattern->pixelArray[col][row] == RgbPixel(RgbPixel::Black);
                                        if (!pixelIsTransparent) {
                                            rgbPatPixel = patternState->pattern->pixelArray[col][row];
                                        }
                                    }
                                    break;
                                case PatternBlendMethod::hsvBlend:
                                case PatternBlendMethod::hsvHueBlend:
                                    if (patternState->pattern->usesHsvModel) {
                                        // TODO:  use hsvTransparent
                                        pixelIsTransparent = patternState->pattern->coneStrings[col][row] == HsvPixel(0, 0, 0);
                                        if (!pixelIsTransparent) {
                                            hsvPatPixel = patternState->pattern->coneStrings[col][row];
                                        }
                                    }
                                    else {
                                        // TODO:  use rgbTransparent
                                        pixelIsTransparent =
                                            patternState->pattern->pixelArray[col][row] == RgbPixel(RgbPixel::Black);
                                        if (!pixelIsTransparent) {
                                            rgb2hsv(patternState->pattern->pixelArray[col][row], hsvPatPixel);
                                        }
                                    }
                                    break;
                            }

                            // Blend the pattern's pixel into the final frame if it isn't transparent.
                            if (!pixelIsTransparent) {
                                anyPixelIsOn = true;
                                switch (patternBlendMethod) {
                                    case PatternBlendMethod::overlay:
                                        // A higher priority pattern overrides a lower priority pattern.
                                        rgbFinalFrame[col][row] = rgbPatPixel;
                                        break;
                                    case PatternBlendMethod::rgbAdd:
                                        // Saturating addition ignores priority, but it looks better than nblend of rgb.
                                        rgbFinalFrame[col][row] += rgbPatPixel;
                                        break;
                                    case PatternBlendMethod::rgbBlend:
                                        // Blending rgb tends to make things look dark when combined.
                                        // TODO:  use RgbPixel::Transparent
                                        if (rgbFinalFrame[col][row] != RgbPixel(RgbPixel::Black)) {
                                            nblend(rgbFinalFrame[col][row],
                                                   rgbPatPixel,
                                                   patternState->amountOfOverlay);
                                        }
                                        else {
                                            rgbFinalFrame[col][row] = rgbPatPixel;
                                        }
                                        break;
                                    case PatternBlendMethod::hsvBlend:
                                    case PatternBlendMethod::hsvHueBlend:
                                        HsvPixel ffPixel = hsvFinalFrame[col][row];
                                        if (hsvFinalFrame[col][row] != HsvPixel(0, 0, 0)) {     // TODO:  use hsvTransparent
                                            nblend(hsvFinalFrame[col][row],
                                                   hsvPatPixel,
                                                   patternState->amountOfOverlay);
                                            if (patternBlendMethod == PatternBlendMethod::hsvHueBlend) {
                                                hsvFinalFrame[col][row].s = max(ffPixel.s, hsvPatPixel.s);
                                                hsvFinalFrame[col][row].v = max(ffPixel.v, hsvPatPixel.v);
                                            }
                                        }
                                        else {
                                            hsvFinalFrame[col][row] = hsvPatPixel;
                                        }
                                        break;
                                }
                            }

                        }
                    }
                }
            }
        }

        if (anyPixelIsOn) {
            if (patternBlendMethod == PatternBlendMethod::hsvBlend || patternBlendMethod == PatternBlendMethod::hsvHueBlend) {
                hsv2rgb(hsvFinalFrame, rgbFinalFrame);
            }
            sendOpcMessage();
        }
        else {
            // Don't allow the cone to go completely dark.
            turnOnSafetyLights();
        }
        doIdlePattern = false;
        timeWentIdle = 0;
    }
    else {
        if (!doIdlePattern) {
            // Turn on the safety lights until the idle pattern takes over.
            turnOnSafetyLights();
            if (timeWentIdle == 0) {
                time(&timeWentIdle);
            }
            else {
                time_t now;
                time(&now);
                // TODO 7/22/2017 ross:  replace the magic number 3 with the hard idle timeout
                if (now - timeWentIdle > 3) {
                    doIdlePattern = true;
                }
            }
        }
        // When doIdlePattern is true, we stop sending messages to the OPC server.
        // The server will display its own rainbow-like pattern when it hasn't
        // received a message for a few seconds.
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

    logMsg(LOG_INFO, "---------- patternController  starting ----------");

    logMsg(LOG_INFO, "numberOfStrings = " + to_string(numberOfStrings));
    logMsg(LOG_INFO, "numberOfPixelsPerString = " + to_string(numberOfPixelsPerString));
    logMsg(LOG_INFO, "pattern blend method is " + patternBlendMethodStr);
    printSchedulePeriods("Shutoff periods", shutoffPeriods);
    printSchedulePeriods("Quiescent periods", quiescentPeriods);

    if (!allocateConePixels<HsvConeStrings, HsvPixelString, HsvPixel>(hsvFinalFrame, numberOfStrings, numberOfPixelsPerString)) {
        logMsg(LOG_ERR, "Unable to allocate pixels for hsvFinalFrame.");
        return(EXIT_FAILURE);
    }

    if (!allocateConePixels<RgbConeStrings, RgbPixelString, RgbPixel>(rgbFinalFrame, numberOfStrings, numberOfPixelsPerString)) {
        logMsg(LOG_ERR, "Unable to allocate pixels for rgbFinalFrame.");
        return(EXIT_FAILURE);
    }

    if (!initOpcBuffer()) {
        return(EXIT_FAILURE);
    }

    // open socket, connect with opc-server
    if (!setUpOpcServerConnection(config.getOpcServerIpAddress())) {
        return(EXIT_FAILURE);
    }

    initWidgets();
    initPatterns();

    logMsg(LOG_INFO, "Initialization done.  Start doing shit!");

    time_t lastPeriodCheckTime = 0;
    bool inPeriod = false;
    string lastPeriodDesc = "";

/*
//=-=-=-=-=-=-=-=-=
    RgbPixel rgb(255, 255, 0);
    //RgbPixel rgb(196, 138, 0);
    cout << "r=" << (int) rgb.r << " g=" << (int) rgb.g << " b=" << (int) rgb.b << endl;
    HsvPixel hsv;
    rgb2hsv(rgb, hsv);
    cout << "h=" << (int) hsv.h << " s=" << (int) hsv.s << " v=" << (int) hsv.v << endl;
    hsv2rgb_rainbow(hsv, rgb);
    cout << "r=" << (int) rgb.r << " g=" << (int) rgb.g << " b=" << (int) rgb.b << endl;
    return EXIT_SUCCESS;
//=-=-=-=-=-=-=-=-=
*/

    // ----- run loop -----
    while (true) {

        // Give the widgets a chance to update their simulated measurements.
        for (auto&& widget : widgets) {
            widget.second->updateSimulatedMeasurements();
        }

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
