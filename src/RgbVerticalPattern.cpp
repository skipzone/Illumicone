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

#include <iostream>

#include "ConfigReader.h"
#include "illumiconePixelUtility.h"
#include "illumiconeUtility.h"
#include "Log.h"
#include "Pattern.h"
#include "RgbVerticalPattern.h"
#include "Widget.h"
#include "WidgetChannel.h"

using namespace std;


extern Log logger;


RgbVerticalPattern::RgbVerticalPattern(const std::string& name)
    : Pattern(name)
    , horizontalVpixelRatio(1)
    , verticalVpixelRatio(1)
    , vpixelArray(&pixelArray)
{
}


RgbVerticalPattern::~RgbVerticalPattern()
{
    if (vpixelArray != &pixelArray) {
        delete vpixelArray;
    }
}


bool RgbVerticalPattern::initPattern(std::map<WidgetId, Widget*>& widgets)
{
    horizontalVpixelRatio = 2;
    verticalVpixelRatio = 1;

    if (!patternConfigObject["widthScaleFactor"].is_number()) {
        logger.logMsg(LOG_ERR, "widthScaleFactor not specified in " + name + " pattern configuration.");
        return false;
    }
    widthScaleFactor = patternConfigObject["widthScaleFactor"].int_value();
    logger.logMsg(LOG_INFO, name + " widthScaleFactor=" + to_string(widthScaleFactor));

    if (!patternConfigObject["maxCyclicalWidth"].is_number()) {
        logger.logMsg(LOG_ERR, "maxCyclicalWidth not specified in " + name + " pattern configuration.");
        return false;
    }
    maxCyclicalWidth = patternConfigObject["maxCyclicalWidth"].int_value();
    logger.logMsg(LOG_INFO, name + " maxCyclicalWidth=" + to_string(maxCyclicalWidth));

    if (!patternConfigObject["widthResetTimeoutSeconds"].is_number()) {
        logger.logMsg(LOG_ERR, "widthResetTimeoutSeconds not specified in " + name + " pattern configuration.");
        return false;
    }
    widthResetTimeoutSeconds = patternConfigObject["widthResetTimeoutSeconds"].int_value();
    logger.logMsg(LOG_INFO, name + " widthResetTimeoutSeconds=" + to_string(widthResetTimeoutSeconds));

    std::vector<Pattern::ChannelConfiguration> channelConfigs = getChannelConfigurations(widgets);
    if (channelConfigs.empty()) {
        logger.logMsg(LOG_ERR, "No valid widget channels are configured for " + name + ".");
        return false;
    }

    for (auto&& channelConfig : channelConfigs) {

        if (channelConfig.inputName == "redPosition") {
            redPositionChannel = channelConfig.widgetChannel;
        }
        else if (channelConfig.inputName == "greenPosition") {
            greenPositionChannel = channelConfig.widgetChannel;
        }
        else if (channelConfig.inputName == "bluePosition") {
            bluePositionChannel = channelConfig.widgetChannel;
        }
        else if (channelConfig.inputName == "width") {
            widthChannel = channelConfig.widgetChannel;
        }
        else {
            logger.logMsg(LOG_WARNING, "inputName '" + channelConfig.inputName
                + "' in input configuration for " + name + " is not recognized.");
            continue;
        }
        logger.logMsg(LOG_INFO, name + " using " + channelConfig.widgetChannel->getName() + " for " + channelConfig.inputName);

        if (channelConfig.measurement != "position") {
            logger.logMsg(LOG_WARNING, name + " supports only position measurements, but the input configuration for "
                + channelConfig.inputName + " doesn't specify position.");
        }
    }

    // ----- initialize object data -----

    rPos = 0;
    gPos = 0;
    bPos = 0;
    nextResetWidthMs = 0;
    resetWidth = true;
    widthPos = 1;

    numVstrings = numStrings * horizontalVpixelRatio,
    pixelsPerVstring = pixelsPerString * verticalVpixelRatio;
    if (horizontalVpixelRatio > 1 || verticalVpixelRatio > 1) {
        vpixelArray = new RgbConeStrings;
        if (!allocateConePixels<RgbConeStrings, RgbPixelString, RgbPixel>(*vpixelArray, numVstrings, pixelsPerVstring))
        {
            logger.logMsg(LOG_ERR, "Virtual pixel allocation failed for " + name + ".");
        }
    }

    return true;
}


bool RgbVerticalPattern::update()
{
    isActive = false;
    bool gotPositionOrWidthUpdate = false;
    unsigned int nowMs = getNowMs();

    if (redPositionChannel != nullptr) {
        if (redPositionChannel->getIsActive()) {
            isActive = true;
            if (redPositionChannel->getHasNewPositionMeasurement()) {
                gotPositionOrWidthUpdate = true;
                rPos = ((unsigned int) redPositionChannel->getPosition()) % numVstrings;
            }
        }
    }

    if (greenPositionChannel != nullptr) {
        if (greenPositionChannel->getIsActive()) {
            isActive = true;
            if (greenPositionChannel->getHasNewPositionMeasurement()) {
                gotPositionOrWidthUpdate = true;
                gPos = ((unsigned int) greenPositionChannel->getPosition()) % numVstrings;
            }
        }
    }

    if (bluePositionChannel != nullptr) {
        if (bluePositionChannel->getIsActive()) {
            isActive = true;
            if (bluePositionChannel->getHasNewPositionMeasurement()) {
                gotPositionOrWidthUpdate = true;
                bPos = ((unsigned int) bluePositionChannel->getPosition()) % numVstrings;
            }
        }
    }

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
                //logger.logMsg(LOG_DEBUG, name + ":  rawWidthPos=" + to_string(rawWidthPos)
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
            logger.logMsg(LOG_DEBUG, name + ":  Resetting width.");
            resetWidth = true;
            widthPos = 1;
        }
    }

    // Draw the stripes.
    if (gotPositionOrWidthUpdate) {

        clearAllPixels(*vpixelArray);

        int leftExtraWidth = 0;
        int rightExtraWidth = 0;
        if (widthPos >= 2) {
            leftExtraWidth = widthPos / 2;
            rightExtraWidth = widthPos - 1 - leftExtraWidth;
        }

        int rWidthLowIndex = rPos - leftExtraWidth;
        int rWidthHighIndex = rPos + rightExtraWidth;

        int gWidthLowIndex = gPos - leftExtraWidth;
        int gWidthHighIndex = gPos + rightExtraWidth;

        int bWidthLowIndex = bPos - leftExtraWidth;
        int bWidthHighIndex = bPos + rightExtraWidth;

        //logger.logMsg(LOG_DEBUG, name
        //    + ":  leftExtraWidth=" + to_string(leftExtraWidth)
        //    + ", rightExtraWidth=" + to_string(rightExtraWidth)
        //    + ", rPos=" + to_string(rPos)
        //    + ", rWidthLowIndex=" + to_string(rWidthLowIndex)
        //    + ", rWidthHighIndex=" + to_string(rWidthHighIndex)
        //    + ", gPos=" + to_string(gPos)
        //    + ", gWidthLowIndex=" + to_string(gWidthLowIndex)
        //    + ", gWidthHighIndex=" + to_string(gWidthHighIndex)
        //    + ", bPos=" + to_string(bPos)
        //    + ", bWidthLowIndex=" + to_string(bWidthLowIndex)
        //    + ", bWidthHighIndex=" + to_string(bWidthHighIndex));

        for (int i = rWidthLowIndex; i <= rWidthHighIndex; ++i) {
            int stringIndex = (i % numVstrings + numVstrings) % numVstrings;
            for (auto&& pixels : vpixelArray->at(stringIndex)) {
                pixels.r = 255;
            }
        }

        for (int i = gWidthLowIndex; i <= gWidthHighIndex; ++i) {
            int stringIndex = (i % numVstrings + numVstrings) % numVstrings;
            for (auto&& pixels : vpixelArray->at(stringIndex)) {
                pixels.g = 255;
            }
        }

        for (int i = bWidthLowIndex; i <= bWidthHighIndex; ++i) {
            int stringIndex = (i % numVstrings + numVstrings) % numVstrings;
            for (auto&& pixels : vpixelArray->at(stringIndex)) {
                pixels.b = 255;
            }
        }

        for (unsigned int col = 0; col < numStrings; col++) {
            unsigned int vcol = col * horizontalVpixelRatio;
            for (unsigned int row = 0; row < pixelsPerString; row++) {
                unsigned int vrow = row * verticalVpixelRatio;
                pixelArray[col][row] = vpixelArray->at(vcol)[vrow];
            }
        }
    }

    return isActive;
}
