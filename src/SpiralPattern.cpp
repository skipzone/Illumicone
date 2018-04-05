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

#include <cmath>

#include "ConfigReader.h"
#include "illumiconePixelUtility.h"
#include "illumiconeUtility.h"
#include "log.h"
#include "Pattern.h"
#include "SpiralPattern.h"
#include "Widget.h"
#include "WidgetChannel.h"


using namespace std;


SpiralPattern::SpiralPattern(const std::string& name)
    : Pattern(name)
{
}


bool SpiralPattern::initPattern(ConfigReader& config, std::map<WidgetId, Widget*>& widgets)
{
    // ----- get input channels -----

    std::vector<Pattern::ChannelConfiguration> channelConfigs = getChannelConfigurations(config, widgets);
    if (channelConfigs.empty()) {
        logMsg(LOG_ERR, "No valid widget channels are configured for " + name + ".");
        return false;
    }

    for (auto&& channelConfig : channelConfigs) {

        if (channelConfig.inputName == "rotation") {
            rotationChannel = channelConfig.widgetChannel;
        }
        else if (channelConfig.inputName == "width") {
            widthChannel = channelConfig.widgetChannel;
        }
        else {
            logMsg(LOG_WARNING, "inputName '" + channelConfig.inputName
                + "' in input configuration for " + name + " is not recognized.");
            continue;
        }
        logMsg(LOG_INFO, name + " using " + channelConfig.widgetChannel->getName() + " for " + channelConfig.inputName);

        if (channelConfig.measurement != "position") {
            logMsg(LOG_WARNING, name + " supports only position measurements, but the input configuration for "
                + channelConfig.inputName + " doesn't specify position.");
        }
    }


    // ----- get pattern configuration -----

    string errMsgSuffix = " in " + name + " pattern configuration.";

    auto patternConfig = config.getPatternConfigJsonObject(name);

    if (!ConfigReader::getIntValue(patternConfig, "widthScaleFactor", widthScaleFactor, errMsgSuffix)) {
        return false;
    }
    logMsg(LOG_INFO, name + " widthScaleFactor=" + to_string(widthScaleFactor));

    if (!ConfigReader::getIntValue(patternConfig, "minCyclicalWidth", minCyclicalWidth, errMsgSuffix)) {
        return false;
    }
    logMsg(LOG_INFO, name + " minCyclicalWidth=" + to_string(minCyclicalWidth));

    if (!ConfigReader::getIntValue(patternConfig, "maxCyclicalWidth", maxCyclicalWidth, errMsgSuffix)) {
        return false;
    }
    logMsg(LOG_INFO, name + " maxCyclicalWidth=" + to_string(maxCyclicalWidth));

    if (!ConfigReader::getIntValue(patternConfig, "widthResetTimeoutSeconds", widthResetTimeoutSeconds, errMsgSuffix)) {
        return false;
    }
    logMsg(LOG_INFO, name + " widthResetTimeoutSeconds=" + to_string(widthResetTimeoutSeconds));


    // ----- initialize object data -----

    nextResetWidthMs = 0;
    resetWidth = true;
    widthPos = 1;

    return true;
}


bool SpiralPattern::update()
{
    isActive = true;    //false;
    bool gotPositionOrWidthUpdate = true;   //false;
    unsigned int nowMs = getNowMs();

//    // Get any updated rotation.
//    if (rotationChannel != nullptr) {
//        if (rotationChannel->getIsActive()) {
//            isActive = true;
//            if (rotationChannel->getHasNewPositionMeasurement()) {
//                gotPositionOrWidthUpdate = true;
//                rPos = ((unsigned int) rotationChannel->getPosition()) % pixelsPerString;
//            }
//        }
//    }

    // Get an updated stripe width, if available.
    if (widthChannel != nullptr) {
        if (widthChannel->getIsActive()) {
            isActive = true;
            if (widthChannel->getHasNewPositionMeasurement()) {
                gotPositionOrWidthUpdate = true;
                int rawWidthPos = widthChannel->getPosition();
                if (resetWidth) {
                    resetWidth = false;
                    widthPosOffset = rawWidthPos;
                }
                widthPos = (rawWidthPos - widthPosOffset) / widthScaleFactor;
                if (maxCyclicalWidth != 0) {
                    // This is a triangle wave function where the period is
                    // (maxCyclicalWidth - 1) * 2 and the range is 1 to maxCyclicalWidth.
                    // We left-shift the wave so that the width starts out at 1.
                    widthPos = abs(abs(widthPos + (maxCyclicalWidth - 1)) % ((maxCyclicalWidth - 1) * 2) - (maxCyclicalWidth - 1)) + 1;
                }
                if (widthPos < 1) {
                    widthPos = 1;
                }
                //logMsg(LOG_DEBUG, name + ":  rawWidthPos=" + to_string(rawWidthPos)
                //                  + ", widthPosOffset=" + to_string(widthPosOffset)
                //                  + ", widthPos=" + to_string(widthPos));
            }
        }
    }

    // Set flag to force width back to 1 when width channel
    // has been inactive for widthResetTimeoutSeconds.
    if (isActive) {
        nextResetWidthMs = 0;
    }
    else {
        if (nextResetWidthMs == 0) {
            nextResetWidthMs = nowMs + widthResetTimeoutSeconds * 1000;
        }
        else if (!resetWidth && (int) (nowMs - nextResetWidthMs) >= 0) {
            //logMsg(LOG_DEBUG, name + ":  Resetting width.");
            resetWidth = true;
            widthPos = 1;
        }
    }


    // Draw.

    if (gotPositionOrWidthUpdate) {

        clearAllPixels(pixelArray);

        // Range of y is [0..1].
        float xStep = 1.0 / numStrings;
        float x = 0;
        for (;;) {

            float y = std::powf(x / 4, 1.5);
            if (y > 1.0) {
                break;
            }

            unsigned int stringIdx = (unsigned int) (x / xStep) % numStrings;
            unsigned int pixelIdx = y * pixelsPerString;
            //logMsg(LOG_DEBUG, name + ":  x=" + to_string(x) + " y=" + to_string(y) + " pixelIdx=" + to_string(pixelIdx));
            pixelArray[stringIdx][pixelIdx].r = 255;

            x += xStep;
        }

    }

    return isActive;
}
