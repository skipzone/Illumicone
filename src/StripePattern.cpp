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
#include "RgbStripePattern.h"
#include "Widget.h"
#include "WidgetChannel.h"

using namespace std;


extern Log logger;


RgbStripePattern::RgbStripePattern(const std::string& name)
    : Pattern(name)
    , stripeIsHorizontal(false)
    , virtualPixelRatio(1)
    , numPixelsInDrawingPlane(numStrings)
    , numVirtualPixelsInDrawingPlane(numStrings)
{
}


bool RgbStripePattern::initPattern(std::map<WidgetId, Widget*>& widgets)
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
        else if (channelConfig.inputName == "position") {
            positionChannel = channelConfig.widgetChannel;
            recognizedChannel = true;
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

    if (!ConfigReader::getIntValue(patternConfigObject, "widthResetTimeoutSeconds", widthResetTimeoutSeconds, errMsgSuffix, 1)) {
        return false;
    }
    logger.logMsg(LOG_INFO, name + " widthResetTimeoutSeconds=" + to_string(widthResetTimeoutSeconds));

    // TODO:  get stripeIsHorizontal

    if (!ConfigReader::getUnsignedIntValue(patternConfigObj, "virtualPixelRatio", virtualPixelRatio, errMsgSuffix, 1)) {
        return false;
    }
    logger.logMsg(LOG_INFO, "%s virtualPixelRatio=%d", name.c_str(), virtualPixelRatio);

    if (stripeIsHorizontal) {
        numPixelsInDrawingPlane = pixelsPerString;
        numVirtualPixelsInDrawingPlane = pixelsPerString * virtualPixelRatio;
    }
    else {
        numPixelsInDrawingPlane = numStrings;
        numVirtualPixelsInDrawingPlane = numStrings * virtualPixelRatio;
    }

    if (!ConfigReader::getIntValue(
        patternConfigObj, "widthScaledownFactor", widthScaledownFactor, errMsgSuffix, 1))
    {
        return false;
    }
    logger.logMsg(LOG_INFO, "%s widthScaledownFactor=%d", name.c_str(), widthScaledownFactor);

    if (!ConfigReader::getIntValue(
        patternConfigObj, "numStripes", numStripes, errMsgSuffix, 1, numStrings / 2))
    {
        return false;
    }
    if (numPixelsInDrawingPlane % numStripes != 0) {
        logger.logMsg(LOG_WARNING, "numStripes in %s pattern configuration should be a factor of %d.",
                      name.c_str(), numPixelsInDrawingPlane);
    }
    logger.logMsg(LOG_INFO, "%s numStripes=%d", name.c_str(), numStripes);
    stripeStep = numPixelsInDrawingPlane / numStripes;

    if (!ConfigReader::getIntValue(
        patternConfigObj, "scaledownFactor", scaledownFactor, errMsgSuffix, 1))
    {
        return false;
    }
    logger.logMsg(LOG_INFO, "%s scaledownFactor=%d", name.c_str(), scaledownFactor);

    if (!ConfigReader::getIntValue(
        patternConfigObj, "minSidebandWidth", minSidebandWidth, errMsgSuffix,
        0, numVirtualPixelsInDrawingPlane / 2))
    {
        return false;
    }
    logger.logMsg(LOG_INFO, "%s minSidebandWidth=%d", name.c_str(), minSidebandWidth);

    if (!ConfigReader::getIntValue(
        patternConfigObj, "maxSidebandWidth", maxSidebandWidth, errMsgSuffix,
        minSidebandWidth, numVirtualPixelsInDrawingPlane / 2))
    {
        return false;
    }
    logger.logMsg(LOG_INFO, "%s maxSidebandWidth=%d", name.c_str(), maxSidebandWidth);

    unsigned int baseIntensityValue;
    if (!ConfigReader::getUnsignedIntValue(
        patternConfigObj, "baseIntensity", baseIntensityValue, errMsgSuffix, 0, 255))
    {
        return false;
    }
    baseIntensity = baseIntensityValue;
    logger.logMsg(LOG_INFO, "%s baseIntensity=%d", name.c_str(), baseIntensity);

    // ----- initialize object data -----

    stripeVPos = 0;
    widthPos = minSidebandWidth;
    nextResetWidthMs = 0;
    resetWidth = true;

    return true;
}


bool RgbStripePattern::update()
{
    isActive = false;
    bool gotPositionOrWidthUpdate = false;
    int rawPosition;
    unsigned int nowMs = getNowMs();

    for (int iColor = 0; iColor < numColors; ++iColor) {
        if (positionChannel[iColor] != nullptr) {
            // Quick and dirty hack to allow other channels to share channel 0's data.
            if (isActive && gotPositionOrWidthUpdate
                && iColor != 0 && positionChannel[iColor] == positionChannel[0])
            {
                for (int iOrient = 0; iOrient < numOrientations; ++iOrient) {
                    if (orientationIsEnabled[iOrient]) {
                        stripeVPos =
                            ((unsigned int) rawPosition) / scaledownFactor
                            % numVirtualPixelsInDrawingPlane;
                    }
                }
            }
            else {
                if (positionChannel[iColor]->getIsActive()) {
                    isActive = true;
                    if (positionChannel[iColor]->getHasNewPositionMeasurement()) {
                        gotPositionOrWidthUpdate = true;
                        rawPosition = positionChannel[iColor]->getPosition();
                        for (int iOrient = 0; iOrient < numOrientations; ++iOrient) {
                            if (orientationIsEnabled[iOrient]) {
                                stripeVPos =
                                    ((unsigned int) rawPosition) / scaledownFactor
                                    % numVirtualPixelsInDrawingPlane;
                            }
                        }
                    }
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

                for (int iColor = 0; iColor < numColors; ++iColor) {
                    for (int iOrient = 0; iOrient < numOrientations; ++iOrient) {
                        if (orientationIsEnabled[iOrient]) {
                            if (maxSidebandWidth > 0) {
                                int scaledWidthPos = (rawWidthPos - widthPosOffset) / widthScaledownFactor[iOrient];
                                // This is a triangle wave function where the period is maxSidebandWidth * 2
                                // and the range is minSidebandWidth to maxSidebandWidth.  We right-shift the
                                // wave so that the width starts out at minSidebandWidth.
                                widthPos =
                                    abs( abs(scaledWidthPos + maxSidebandWidth)
                                         % (maxSidebandWidth * 2) - maxSidebandWidth )
                                    + minSidebandWidth;
                            }
                            //logger.logMsg(LOG_DEBUG, name
                            //              + ":  iColor=" + to_string(iColor)
                            //              + ", iOrient=" + to_string(iOrient)
                            //              + ", rawWidthPos=" + to_string(rawWidthPos)
                            //              + ", widthPosOffset=" + to_string(widthPosOffset)
                            //              + ", scaledWidthPos=" + to_string(scaledWidthPos));
                            //              + ", widthPos=" + to_string(widthPos));
                        }
                    }
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
            for (int iOrient = 0; iOrient < numOrientations; ++iOrient) {
                for (int iColor = 0; iColor < numColors; ++iColor) {
                    widthPos = minSidebandWidth;
                }
            }
        }
    }

    // Draw the stripes.
    if (gotPositionOrWidthUpdate) {

        clearAllPixels(pixelArray);

        for (int iColor = 0; iColor < numColors; ++iColor) {
            if (positionChannel[iColor] != nullptr) {
                for (int iOrient = 0; iOrient < numOrientations; ++iOrient) {
                    if (orientationIsEnabled[iOrient]) {

                    int sidebandWidth = widthPos;
                    float intensitySlope = (float) baseIntensity / (float) (sidebandWidth + 1);

                    int lowVEl = stripeVPos - sidebandWidth;
                    int highVEl = stripeVPos + sidebandWidth;

                    switch (iOrient) {

                        case vert:

                            //logger.logMsg(LOG_DEBUG, name
                            //    + " vert:  iColor=" + to_string(iColor)
                            //    + ",  widthPos=" + to_string(widthPos)
                            //    + ",  sidebandWidth=" + to_string(sidebandWidth)
                            //    + ", intensitySlope=" + to_string(intensitySlope)
                            //    + ", stripeVPos=" + to_string(stripeVPos)
                            //    + ", lowVEl=" + to_string(lowVEl)
                            //    + ", highVEl=" + to_string(highVEl));

                            for (int i = lowVEl; i <= highVEl; ++i) {
                                unsigned int vStringIndex = ((i + numVirtualPixelsInDrawingPlane) % numVirtualPixelsInDrawingPlane);
                                // If this virtual string corresponds to a physical
                                // string, we'll draw into the physical string.
                                if (vStringIndex % virtualPixelRatio == 0) {
                                    unsigned int stringIndex = vStringIndex / virtualPixelRatio;
                                    float distanceToCenter = abs(i - stripeVPos);
                                    uint8_t intensity =
                                        baseIntensity - (uint8_t) (intensitySlope * distanceToCenter);
                                    for (int iStripe = 0; iStripe < numStripes; ++iStripe) {
                                        for (auto&& pixels : pixelArray[stringIndex]) {
                                            pixels.raw[iColor] = qadd8(pixels.raw[iColor], intensity);
                                        }
                                        stringIndex = (stringIndex + stripeStep) % numStrings;
                                    }
                                }
                            }

                            break;

                        case horiz:

                            //logger.logMsg(LOG_DEBUG, name
                            //    + " horiz:  iColor=" + to_string(iColor)
                            //    + ",  widthPos=" + to_string(widthPos[iColor])
                            //    + ",  sidebandWidth=" + to_string(sidebandWidth)
                            //    + ", intensitySlope=" + to_string(intensitySlope)
                            //    + ", stripeVPos=" + to_string(stripeVPos)
                            //    + ", lowVEl=" + to_string(lowVEl)
                            //    + ", highVEl=" + to_string(highVEl));

                            for (int i = lowVEl; i <= highVEl; ++i) {
                                unsigned int virtualPixelIdx =
                                    ((i + numVirtualPixelsInDrawingPlane) % numVirtualPixelsInDrawingPlane);
                                // If this virtual pixel corresponds to a physical
                                // pixel, we'll draw into the physical pixels.
                                if (virtualPixelIdx % virtualPixelRatio == 0) {
                                    unsigned int pixelIndex = virtualPixelIdx / virtualPixelRatio;
                                    float distanceToCenter = abs(i - stripeVPos);
                                    uint8_t intensity =
                                        baseIntensity - (uint8_t) (intensitySlope * distanceToCenter);
                                    for (int iStripe = 0; iStripe < numStripes; ++iStripe) {
                                        for (auto&& stringPixels : pixelArray) {
                                            stringPixels[pixelIndex].raw[iColor] =
                                                qadd8(stringPixels[pixelIndex].raw[iColor], intensity);
                                        }
                                        pixelIndex = (pixelIndex + stripeStep) % pixelsPerString;
                                    }
                                }
                            }

                            break;
                        }
                    }
                }
            }
        }
    }

    return isActive;
}

