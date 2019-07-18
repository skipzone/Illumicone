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

    for (int iColor = 0; iColor < numColors; ++iColor) {
        string elName = rgbPrefix[iColor] + string("NumStripes");
        if (!ConfigReader::getIntValue(patternConfigObject, elName, numStripes[iColor], errMsgSuffix, 1, numVstrings / 2)) {
            return false;
        }
        if (numVstrings % numStripes[iColor] != 0) {
            logger.logMsg(LOG_ERR, "%s in %s pattern configuration must be a factor of %d.",
                          elName.c_str(), name.c_str(), numVstrings);
            return false;
        }
        logger.logMsg(LOG_INFO, "%s %s=%d", name.c_str(), elName.c_str(), numStripes[iColor]);
        stripeStep[iColor] = numVstrings / numStripes[iColor];
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

    // TODO:  need to support min. sideband width 0
    for (int iColor = 0; iColor < numColors; ++iColor) {
        string elName = rgbPrefix[iColor] + string("MinSidebandWidth");
        if (!ConfigReader::getIntValue(patternConfigObject, elName, minSidebandWidth[iColor], errMsgSuffix,
                                       1, numVstrings / 2))
        {
            return false;
        }
        logger.logMsg(LOG_INFO, "%s %s=%d", name.c_str(), elName.c_str(), minSidebandWidth[iColor]);
    }

    for (int iColor = 0; iColor < numColors; ++iColor) {
        string elName = rgbPrefix[iColor] + string("MaxSidebandWidth");
        if (!ConfigReader::getIntValue(patternConfigObject, elName, maxSidebandWidth[iColor], errMsgSuffix,
                                       minSidebandWidth[iColor], numVstrings / 2))
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
        stripePos[iColor] = 0;
        widthPos[iColor] = minSidebandWidth[iColor];
    }
    nextResetWidthMs = 0;
    resetWidth = true;

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

    for (int iColor = 0; iColor < numColors; ++iColor) {
        if (positionChannel[iColor] != nullptr) {
            if (positionChannel[iColor]->getIsActive()) {
                isActive = true;
                if (positionChannel[iColor]->getHasNewPositionMeasurement()) {
                    gotPositionOrWidthUpdate = true;
                    stripePos[iColor] =
                        ((unsigned int) positionChannel[iColor]->getPosition()) / scaledownFactor[iColor] % numVstrings;
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

                // This is a triangle wave function where the period is
                // (maxSidebandWidth - 1) * 2 and the range is 1 to maxSidebandWidth.
                // We left-shift the wave so that the width starts out at 1.
                for (int iColor = 0; iColor < numColors; ++iColor) {
                    widthPos[iColor] = abs(abs(scaledWidthPos + (maxSidebandWidth[iColor] - 1))
                                           % ((maxSidebandWidth[iColor] - 1) * 2) - (maxSidebandWidth[iColor] - 1))
                                       + minSidebandWidth[iColor];
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
                widthPos[iColor] = minSidebandWidth[iColor];
            }
        }
    }

    // Draw the stripes.
    if (gotPositionOrWidthUpdate) {

        clearAllPixels(*vpixelArray);

        for (int iColor = 0; iColor < numColors; ++iColor) {
            if (positionChannel[iColor] != nullptr) {

                int sidebandWidth = widthPos[iColor] - 1;
                float intensitySlope = 255.0 / (sidebandWidth + 1);
                int lowString = stripePos[iColor] - sidebandWidth;
                int highString = stripePos[iColor] + sidebandWidth;

                //logger.logMsg(LOG_DEBUG, name
                //    + ":  iColor=" + to_string(iColor)
                //    + ",  widthPos[iColor]=" + to_string(widthPos[iColor])
                //    + ",  sidebandWidth=" + to_string(sidebandWidth)
                //    + ", intensitySlope=" + to_string(intensitySlope)
                //    + ", stripePos[iColor]=" + to_string(stripePos[iColor])
                //    + ", lowString=" + to_string(lowString)
                //    + ", highString=" + to_string(highString));

                for (int i = lowString; i <= highString; ++i) {
                    float distanceToCenter = abs(i - stripePos[iColor]);
                    uint8_t intensity = 255 - (uint8_t) (intensitySlope * distanceToCenter);
                    int stringIndex = ((i + numVstrings) % numVstrings);
                    for (int iStripe = 0; iStripe < numStripes[iColor]; ++iStripe) {
                        for (auto&& pixels : vpixelArray->at(stringIndex)) {
                            pixels.raw[iColor] = intensity;
                        }
                        stringIndex = (stringIndex + stripeStep[iColor]) % numVstrings;
                    }
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

