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
    // ----- get input channels -----

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

    // ----- get pattern configuration -----

    string errMsgSuffix = " in " + name + " pattern configuration.";

    if (!ConfigReader::getUnsignedIntValue(patternConfigObject, "horizontalVpixelRatio", horizontalVpixelRatio, errMsgSuffix, 1)) {
        return false;
    }
    if ((horizontalVpixelRatio & (horizontalVpixelRatio - 1)) != 0) {
        logger.logMsg(LOG_ERR, "horizontalVpixelRatio in " + name + " pattern configuration must be a power of 2.");
        return false;
    }
    logger.logMsg(LOG_INFO, name + " horizontalVpixelRatio=" + to_string(horizontalVpixelRatio));

    if (!ConfigReader::getUnsignedIntValue(patternConfigObject, "verticalVpixelRatio", verticalVpixelRatio, errMsgSuffix, 1)) {
        return false;
    }
    if ((verticalVpixelRatio & (verticalVpixelRatio - 1)) != 0) {
        logger.logMsg(LOG_ERR, "verticalVpixelRatio in " + name + " pattern configuration must be a power of 2.");
        return false;
    }
    logger.logMsg(LOG_INFO, name + " verticalVpixelRatio=" + to_string(verticalVpixelRatio));

    numVstrings = numStrings * horizontalVpixelRatio,
    pixelsPerVstring = pixelsPerString * verticalVpixelRatio;

    if (!ConfigReader::getIntValue(patternConfigObject, "rScaledownFactor", rScaledownFactor, errMsgSuffix, 1)) {
        return false;
    }
    logger.logMsg(LOG_INFO, name + " rScaledownFactor=" + to_string(rScaledownFactor));

    if (!ConfigReader::getIntValue(patternConfigObject, "gScaledownFactor", gScaledownFactor, errMsgSuffix, 1)) {
        return false;
    }
    logger.logMsg(LOG_INFO, name + " gScaledownFactor=" + to_string(gScaledownFactor));

    if (!ConfigReader::getIntValue(patternConfigObject, "bScaledownFactor", bScaledownFactor, errMsgSuffix, 1)) {
        return false;
    }
    logger.logMsg(LOG_INFO, name + " bScaledownFactor=" + to_string(bScaledownFactor));

    if (!ConfigReader::getIntValue(patternConfigObject, "widthScaledownFactor", widthScaledownFactor, errMsgSuffix, 1)) {
        return false;
    }
    logger.logMsg(LOG_INFO, name + " widthScaledownFactor=" + to_string(widthScaledownFactor));

    // TODO:  need to support min. sideband width 0
    if (!ConfigReader::getIntValue(patternConfigObject, "minSidebandWidth", minSidebandWidth, errMsgSuffix,
                                   1, numVstrings / 2))
    {
        return false;
    }
    logger.logMsg(LOG_INFO, name + " minSidebandWidth=" + to_string(minSidebandWidth));

    if (!ConfigReader::getIntValue(patternConfigObject, "maxSidebandWidth", maxSidebandWidth, errMsgSuffix,
                                   minSidebandWidth, numVstrings / 2))
    {
        return false;
    }
    logger.logMsg(LOG_INFO, name + " maxSidebandWidth=" + to_string(maxSidebandWidth));

    if (!ConfigReader::getIntValue(patternConfigObject, "widthResetTimeoutSeconds", widthResetTimeoutSeconds, errMsgSuffix, 1)) {
        return false;
    }
    logger.logMsg(LOG_INFO, name + " widthResetTimeoutSeconds=" + to_string(widthResetTimeoutSeconds));

    // ----- initialize object data -----

    rPos = 0;
    gPos = 0;
    bPos = 0;
    nextResetWidthMs = 0;
    resetWidth = true;
    widthPos = minSidebandWidth;

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
                rPos = ((unsigned int) redPositionChannel->getPosition()) / rScaledownFactor % numVstrings;
            }
        }
    }

    if (greenPositionChannel != nullptr) {
        if (greenPositionChannel->getIsActive()) {
            isActive = true;
            if (greenPositionChannel->getHasNewPositionMeasurement()) {
                gotPositionOrWidthUpdate = true;
                gPos = ((unsigned int) greenPositionChannel->getPosition()) / gScaledownFactor % numVstrings;
            }
        }
    }

    if (bluePositionChannel != nullptr) {
        if (bluePositionChannel->getIsActive()) {
            isActive = true;
            if (bluePositionChannel->getHasNewPositionMeasurement()) {
                gotPositionOrWidthUpdate = true;
                bPos = ((unsigned int) bluePositionChannel->getPosition()) / bScaledownFactor % numVstrings;
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
                int scaledWidthPos = (rawWidthPos - widthPosOffset) / widthScaledownFactor;
                // This is a triangle wave function where the period is
                // (maxSidebandWidth - 1) * 2 and the range is 1 to maxSidebandWidth.
                // We left-shift the wave so that the width starts out at 1.
                widthPos = abs(abs(scaledWidthPos + (maxSidebandWidth - 1)) % ((maxSidebandWidth - 1) * 2) - (maxSidebandWidth - 1))
                           + minSidebandWidth;
                //logger.logMsg(LOG_DEBUG, name + ":  rawWidthPos=" + to_string(rawWidthPos)
                //                  + ", widthPosOffset=" + to_string(widthPosOffset)
                //                  + ", scaledWidthPos=" + to_string(scaledWidthPos)
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
            widthPos = minSidebandWidth;
        }
    }

    // Draw the stripes.
    if (gotPositionOrWidthUpdate) {

        clearAllPixels(*vpixelArray);

        int sidebandWidth = widthPos - 1;

        float intensitySlope = 255.0 / (sidebandWidth + 1);

        int rLow = rPos - sidebandWidth;
        int rHigh = rPos + sidebandWidth;

        int gLow = gPos - sidebandWidth;
        int gHigh = gPos + sidebandWidth;

        int bLow = bPos - sidebandWidth;
        int bHigh = bPos + sidebandWidth;

/*
        logger.logMsg(LOG_DEBUG, name
            + ":  widthPos=" + to_string(widthPos)
            + ":  sidebandWidth=" + to_string(sidebandWidth)
            + ", intensitySlope=" + to_string(intensitySlope)
            + ", rPos=" + to_string(rPos)
            + ", rLow=" + to_string(rLow)
            + ", rHigh=" + to_string(rHigh)
            + ", gPos=" + to_string(gPos)
            + ", gLow=" + to_string(gLow)
            + ", gHigh=" + to_string(gHigh)
            + ", bPos=" + to_string(bPos)
            + ", bLow=" + to_string(bLow)
            + ", bHigh=" + to_string(bHigh));
*/

        if (redPositionChannel != nullptr) {
            for (int i = rLow; i <= rHigh; ++i) {
                float distanceToCenter = abs(i - rPos);
                uint8_t intensity = 255 - (uint8_t) (intensitySlope * distanceToCenter);
                int stringIndex = ((i + numVstrings) % numVstrings);
                for (auto&& pixels : vpixelArray->at(stringIndex)) {
                    pixels.r = intensity;
                }
            }
        }

        if (greenPositionChannel != nullptr) {
            for (int i = gLow; i <= gHigh; ++i) {
                float distanceToCenter = abs(i - gPos);
                uint8_t intensity = 255 - (uint8_t) (intensitySlope * distanceToCenter);
                int stringIndex = ((i + numVstrings) % numVstrings);
                for (auto&& pixels : vpixelArray->at(stringIndex)) {
                    pixels.g = intensity;
                }
            }
        }

        if (bluePositionChannel != nullptr) {
            for (int i = bLow; i <= bHigh; ++i) {
                float distanceToCenter = abs(i - bPos);
                uint8_t intensity = 255 - (uint8_t) (intensitySlope * distanceToCenter);
                int stringIndex = ((i + numVstrings) % numVstrings);
                for (auto&& pixels : vpixelArray->at(stringIndex)) {
                    pixels.b = intensity;
                }
            }
        }

        if (horizontalVpixelRatio > 1 || verticalVpixelRatio > 1) {
            // Map virtual pixels onto physical pixels.
            for (unsigned int col = 0; col < numStrings; col++) {
                unsigned int vcol = col * horizontalVpixelRatio;
                for (unsigned int row = 0; row < pixelsPerString; row++) {
                    unsigned int vrow = row * verticalVpixelRatio;
                    pixelArray[col][row] = vpixelArray->at(vcol)[vrow];
                }
            }
        }
    }

    return isActive;
}

