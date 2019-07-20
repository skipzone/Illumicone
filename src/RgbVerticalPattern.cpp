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


constexpr char RgbVerticalPattern::rgbPrefix[];


RgbVerticalPattern::RgbVerticalPattern(const std::string& name)
    : Pattern(name)
    , horizontalVPixelRatio(1)
{
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
        bool recognizedChannel = false;

        if (channelConfig.inputName == "width") {
            widthChannel = channelConfig.widgetChannel;
            recognizedChannel = true;
        }

        for (int iColor = 0; !recognizedChannel && iColor < numColors; ++iColor) {
            string inputName = rgbPrefix[iColor] + string("Position");
            if (channelConfig.inputName == inputName) {
                positionChannel[iColor] = channelConfig.widgetChannel;
                recognizedChannel = true;
            }
        }

        if (recognizedChannel) {
            logger.logMsg(LOG_INFO, name + " using " + channelConfig.widgetChannel->getName() + " for " + channelConfig.inputName);
            if (channelConfig.measurement != "position") {
                logger.logMsg(LOG_WARNING, name + " supports only position measurements, but the input configuration for "
                              + channelConfig.inputName + " doesn't specify position.");
            }
        }
        else {
            logger.logMsg(LOG_WARNING, "inputName '" + channelConfig.inputName
                          + "' in input configuration for " + name + " is not recognized.");
        }
    }

    // ----- get pattern configuration -----

    string errMsgSuffix = " in " + name + " pattern configuration.";

    if (!ConfigReader::getUnsignedIntValue(patternConfigObject, "horizontalVPixelRatio", horizontalVPixelRatio, errMsgSuffix, 1)) {
        return false;
    }
    if ((horizontalVPixelRatio & (horizontalVPixelRatio - 1)) != 0) {
        logger.logMsg(LOG_ERR, "horizontalVPixelRatio in " + name + " pattern configuration must be a power of 2.");
        return false;
    }
    logger.logMsg(LOG_INFO, name + " horizontalVPixelRatio=" + to_string(horizontalVPixelRatio));
    numVStrings = numStrings * horizontalVPixelRatio;

    for (int iColor = 0; iColor < numColors; ++iColor) {
        string elName = rgbPrefix[iColor] + string("NumStripes");
        if (!ConfigReader::getIntValue(patternConfigObject, elName, numStripes[iColor], errMsgSuffix, 1, numStrings / 2)) {
            return false;
        }
        if (numStrings % numStripes[iColor] != 0) {
            logger.logMsg(LOG_ERR, "%s in %s pattern configuration must be a factor of %d.",
                          elName.c_str(), name.c_str(), numStrings);
            return false;
        }
        logger.logMsg(LOG_INFO, "%s %s=%d", name.c_str(), elName.c_str(), numStripes[iColor]);
        stripeStep[iColor] = numStrings / numStripes[iColor];
    }

    for (int iColor = 0; iColor < numColors; ++iColor) {
        string elName = rgbPrefix[iColor] + string("ScaledownFactor");
        if (!ConfigReader::getIntValue(patternConfigObject, elName, scaledownFactor[iColor], errMsgSuffix, 1)) {
            return false;
        }
        logger.logMsg(LOG_INFO, "%s %s=%d", name.c_str(), elName.c_str(), scaledownFactor[iColor]);
    }

    if (!ConfigReader::getIntValue(patternConfigObject, "widthScaledownFactor", widthScaledownFactor, errMsgSuffix, 1)) {
        return false;
    }
    logger.logMsg(LOG_INFO, name + " widthScaledownFactor=" + to_string(widthScaledownFactor));

    for (int iColor = 0; iColor < numColors; ++iColor) {
        string elName = rgbPrefix[iColor] + string("MinSidebandWidth");
        if (!ConfigReader::getIntValue(patternConfigObject, elName, minSidebandWidth[iColor], errMsgSuffix,
                                       0, numVStrings / 2))
        {
            return false;
        }
        logger.logMsg(LOG_INFO, "%s %s=%d", name.c_str(), elName.c_str(), minSidebandWidth[iColor]);
    }

    for (int iColor = 0; iColor < numColors; ++iColor) {
        string elName = rgbPrefix[iColor] + string("MaxSidebandWidth");
        if (!ConfigReader::getIntValue(patternConfigObject, elName, maxSidebandWidth[iColor], errMsgSuffix,
                                       minSidebandWidth[iColor], numVStrings / 2))
        {
            return false;
        }
        logger.logMsg(LOG_INFO, "%s %s=%d", name.c_str(), elName.c_str(), maxSidebandWidth[iColor]);
    }

    if (!ConfigReader::getIntValue(patternConfigObject, "widthResetTimeoutSeconds", widthResetTimeoutSeconds, errMsgSuffix, 1)) {
        return false;
    }
    logger.logMsg(LOG_INFO, name + " widthResetTimeoutSeconds=" + to_string(widthResetTimeoutSeconds));

    // ----- initialize object data -----

    for (int iColor = 0; iColor < numColors; ++iColor) {
        stripeVPos[iColor] = 0;
        widthPos[iColor] = 0;
    }
    nextResetWidthMs = 0;
    resetWidth = true;

    return true;
}


bool RgbVerticalPattern::update()
{
    isActive = false;
    bool gotPositionOrWidthUpdate = false;
    unsigned int nowMs = getNowMs();

    for (int iColor = 0; iColor < numColors; ++iColor) {
        if (positionChannel[iColor] != nullptr) {
            if (positionChannel[iColor]->getIsActive()) {
                isActive = true;
                if (positionChannel[iColor]->getHasNewPositionMeasurement()) {
                    gotPositionOrWidthUpdate = true;
                    stripeVPos[iColor] =
                        ((unsigned int) positionChannel[iColor]->getPosition()) / scaledownFactor[iColor] % numVStrings;
                }
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
                //logger.logMsg(LOG_DEBUG, name
                //                         + ":  rawWidthPos=" + to_string(rawWidthPos)
                //                         + ", widthPosOffset=" + to_string(widthPosOffset)
                //                         + ", scaledWidthPos=" + to_string(scaledWidthPos));

                for (int iColor = 0; iColor < numColors; ++iColor) {
                    if (maxSidebandWidth[iColor] > 0) {
                        // This is a triangle wave function where the period is maxSidebandWidth * 2
                        // and the range is minSidebandWidth to maxSidebandWidth.  We right-shift the
                        // wave so that the width starts out at minSidebandWidth.
                        widthPos[iColor] = abs(abs(scaledWidthPos + maxSidebandWidth[iColor])
                                               % (maxSidebandWidth[iColor] * 2) - maxSidebandWidth[iColor])
                                           + minSidebandWidth[iColor];
                    }
                    //logger.logMsg(LOG_DEBUG, name
                    //                  + ":  iColor=" + to_string(iColor)
                    //                  + ", widthPos[iColor]=" + to_string(widthPos[iColor]));
                }
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
            for (int iColor = 0; iColor < numColors; ++iColor) {
                widthPos[iColor] = 0;
            }
        }
    }

    // Draw the stripes.
    if (gotPositionOrWidthUpdate) {

        clearAllPixels(pixelArray);

        for (int iColor = 0; iColor < numColors; ++iColor) {
            if (positionChannel[iColor] != nullptr) {

                int sidebandWidth = widthPos[iColor];
                float intensitySlope = 255.0 / (sidebandWidth + 1);
                int lowVString = stripeVPos[iColor] - sidebandWidth;
                int highVString = stripeVPos[iColor] + sidebandWidth;

                //logger.logMsg(LOG_DEBUG, name
                //    + ":  iColor=" + to_string(iColor)
                //    + ",  widthPos[iColor]=" + to_string(widthPos[iColor])
                //    + ",  sidebandWidth=" + to_string(sidebandWidth)
                //    + ", intensitySlope=" + to_string(intensitySlope)
                //    + ", stripeVPos[iColor]=" + to_string(stripeVPos[iColor])
                //    + ", lowVString=" + to_string(lowVString)
                //    + ", highVString=" + to_string(highVString));

                for (int i = lowVString; i <= highVString; ++i) {
                    unsigned int vstringIndex = ((i + numVStrings) % numVStrings);
                    // If this virtual string corresponds to a physical
                    // string we'll draw into the physical string.
                    if (vstringIndex % horizontalVPixelRatio == 0) {
                        unsigned int stringIndex = vstringIndex / horizontalVPixelRatio;
                        float distanceToCenter = abs(i - stripeVPos[iColor]);
                        uint8_t intensity = 255 - (uint8_t) (intensitySlope * distanceToCenter);
                        for (int iStripe = 0; iStripe < numStripes[iColor]; ++iStripe) {
                            for (auto&& pixels : pixelArray[stringIndex]) {
                                pixels.raw[iColor] = intensity;
                            }
                            stringIndex = (stringIndex + stripeStep[iColor]) % numStrings;
                        }
                    }
                }
            }
        }
    }

    return isActive;
}

