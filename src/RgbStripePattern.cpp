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


constexpr char RgbStripePattern::rgbPrefix[];
constexpr char RgbStripePattern::orientationPrefix[];


RgbStripePattern::RgbStripePattern(const std::string& name)
    : Pattern(name)
    , vPixelsPerString(pixelsPerString)
    , numVStrings(numStrings)
    , horizontalVPixelRatio(1)
    , verticalVPixelRatio(1)
{
    numPixelsForOrientation[horiz] = numStrings;
    numPixelsForOrientation[vert] = pixelsPerString;
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

    for (int iOrient = 0; iOrient < numOrientations; ++iOrient) {

        string elName = orientationPrefix[iOrient] + string("Enable");
        if (!ConfigReader::getBoolValue(patternConfigObject, elName, orientationIsEnabled[iOrient], errMsgSuffix)) {
            return false;
        }
        logger.logMsg(LOG_INFO, "%s %s=%d", name.c_str(), elName.c_str(), orientationIsEnabled[iOrient]);

        if (orientationIsEnabled[iOrient]) {

            string orientConfigName = orientationPrefix[iOrient] + string("Config");
            json11::Json orientConfigObj;
            if (!ConfigReader::getJsonObject(patternConfigObject,
                                             orientConfigName,
                                             orientConfigObj,
                                             errMsgSuffix))
            {
                return false;
            }

            unsigned int vPixelRatio;
            if (!ConfigReader::getUnsignedIntValue(orientConfigObj, "virtualPixelRatio", vPixelRatio, errMsgSuffix, 1)) {
                return false;
            }
            logger.logMsg(LOG_INFO, "%s %s virtualPixelRatio=%d", name.c_str(), orientConfigName.c_str(), vPixelRatio);
            switch (iOrient) {
                case horiz:
                    verticalVPixelRatio = vPixelRatio;
                    vPixelsPerString = pixelsPerString * verticalVPixelRatio;
                    numPixelsForOrientation[horiz] = pixelsPerString;
                    numVPixelsForOrientation[horiz] = vPixelsPerString;
                    break;
                case vert:
                    horizontalVPixelRatio = vPixelRatio;
                    numVStrings = numStrings * horizontalVPixelRatio;
                    numPixelsForOrientation[vert] = numStrings;
                    numVPixelsForOrientation[vert] = numVStrings;
                    break;
                default:
                    logger.logMsg(LOG_ERR, "Unsupported orientation %d in %s %s pattern configuration.",
                                  iOrient, name.c_str(), orientConfigName.c_str());

            }

            if (!ConfigReader::getIntValue(orientConfigObj, "widthScaledownFactor", widthScaledownFactor[iOrient], errMsgSuffix, 1)) {
                return false;
            }
            logger.logMsg(LOG_INFO, "%s %s widthScaledownFactor=%d",
                          name.c_str(), orientConfigName.c_str(), widthScaledownFactor[iOrient]);

            for (int iColor = 0; iColor < numColors; ++iColor) {
                string elName;

                elName = rgbPrefix[iColor] + string("NumStripes");
                if (!ConfigReader::getIntValue(
                    orientConfigObj, elName, numStripes[iOrient][iColor], errMsgSuffix, 1, numStrings / 2))
                {
                    return false;
                }
                if (numPixelsForOrientation[iOrient] % numStripes[iOrient][iColor] != 0) {
                    logger.logMsg(LOG_WARNING, "%s in %s %s pattern configuration should be a factor of %d.",
                                  elName.c_str(), name.c_str(), orientConfigName.c_str(), numPixelsForOrientation[iOrient]);
                }
                logger.logMsg(LOG_INFO, "%s %s %s=%d",
                              name.c_str(), orientConfigName.c_str(), elName.c_str(), numStripes[iOrient][iColor]);
                stripeStep[iOrient][iColor] = numPixelsForOrientation[iOrient] / numStripes[iOrient][iColor];

                elName = rgbPrefix[iColor] + string("ScaledownFactor");
                if (!ConfigReader::getIntValue(orientConfigObj, elName, scaledownFactor[iOrient][iColor], errMsgSuffix, 1)) {
                    return false;
                }
                logger.logMsg(LOG_INFO, "%s %s %s=%d",
                              name.c_str(), orientConfigName.c_str(), elName.c_str(), scaledownFactor[iOrient][iColor]);

                elName = rgbPrefix[iColor] + string("MinSidebandWidth");
                if (!ConfigReader::getIntValue(
                    orientConfigObj, elName, minSidebandWidth[iOrient][iColor], errMsgSuffix,
                    0, numVPixelsForOrientation[iOrient] / 2))
                {
                    return false;
                }
                logger.logMsg(LOG_INFO, "%s %s %s=%d",
                              name.c_str(), orientConfigName.c_str(), elName.c_str(), minSidebandWidth[iOrient][iColor]);

                elName = rgbPrefix[iColor] + string("MaxSidebandWidth");
                if (!ConfigReader::getIntValue(
                    orientConfigObj, elName, maxSidebandWidth[iOrient][iColor], errMsgSuffix,
                    minSidebandWidth[iOrient][iColor], numVPixelsForOrientation[iOrient] / 2))
                {
                    return false;
                }
                logger.logMsg(LOG_INFO, "%s %s %s=%d",
                              name.c_str(), orientConfigName.c_str(), elName.c_str(), maxSidebandWidth[iOrient][iColor]);
            }
        }
    }

    if (!ConfigReader::getIntValue(patternConfigObject, "widthResetTimeoutSeconds", widthResetTimeoutSeconds, errMsgSuffix, 1)) {
        return false;
    }
    logger.logMsg(LOG_INFO, name + " widthResetTimeoutSeconds=" + to_string(widthResetTimeoutSeconds));

    // ----- initialize object data -----

    for (int iOrient = 0; iOrient < numOrientations; ++iOrient) {
        for (int iColor = 0; iColor < numColors; ++iColor) {
            stripeVPos[iOrient][iColor] = 0;
            widthPos[iOrient][iColor] = 0;
        }
    }
    nextResetWidthMs = 0;
    resetWidth = true;

    return true;
}


bool RgbStripePattern::update()
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
                    for (int iOrient = 0; iOrient < numOrientations; ++iOrient) {
                        if (orientationIsEnabled[iOrient]) {
                            stripeVPos[iOrient][iColor] =
                                ((unsigned int) positionChannel[iColor]->getPosition()) / scaledownFactor[iOrient][iColor]
                                % numPixelsForOrientation[iOrient];
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
                            if (maxSidebandWidth[iOrient][iColor] > 0) {
                                int scaledWidthPos = (rawWidthPos - widthPosOffset) / widthScaledownFactor[iOrient];
                                // This is a triangle wave function where the period is maxSidebandWidth * 2
                                // and the range is minSidebandWidth to maxSidebandWidth.  We right-shift the
                                // wave so that the width starts out at minSidebandWidth.
                                widthPos[iOrient][iColor] =
                                    abs( abs(scaledWidthPos + maxSidebandWidth[iOrient][iColor])
                                         % (maxSidebandWidth[iOrient][iColor] * 2) - maxSidebandWidth[iOrient][iColor] )
                                    + minSidebandWidth[iOrient][iColor];
                            }
                            //logger.logMsg(LOG_DEBUG, name
                            //              + ":  iColor=" + to_string(iColor)
                            //              + ", iOrient=" + to_string(iOrient)
                            //              + ", rawWidthPos=" + to_string(rawWidthPos)
                            //              + ", widthPosOffset=" + to_string(widthPosOffset)
                            //              + ", scaledWidthPos=" + to_string(scaledWidthPos));
                            //              + ", widthPos[iOrient][iColor]=" + to_string(widthPos[iOrient][iColor]));
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
                    widthPos[iOrient][iColor] = 0;
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

                    int sidebandWidth = widthPos[iOrient][iColor];
                    float intensitySlope = 255.0 / (sidebandWidth + 1);

                    int lowVEl = stripeVPos[iOrient][iColor] - sidebandWidth;
                    int highVEl = stripeVPos[iOrient][iColor] + sidebandWidth;

                    switch (iOrient) {

                        case vert:

                            //logger.logMsg(LOG_DEBUG, name
                            //    + " vert:  iColor=" + to_string(iColor)
                            //    + ",  widthPos[iColor]=" + to_string(widthPos[iColor])
                            //    + ",  sidebandWidth=" + to_string(sidebandWidth)
                            //    + ", intensitySlope=" + to_string(intensitySlope)
                            //    + ", stripeVPos[iOrient][iColor]=" + to_string(stripeVPos[iOrient][iColor])
                            //    + ", lowVEl=" + to_string(lowVEl)
                            //    + ", highVEl=" + to_string(highVEl));

                            for (int i = lowVEl; i <= highVEl; ++i) {
                                unsigned int vStringIndex = ((i + numVStrings) % numVStrings);
                                // If this virtual string corresponds to a physical
                                // string, we'll draw into the physical string.
                                if (vStringIndex % horizontalVPixelRatio == 0) {
                                    unsigned int stringIndex = vStringIndex / horizontalVPixelRatio;
                                    float distanceToCenter = abs(i - stripeVPos[iOrient][iColor]);
                                    uint8_t intensity = 255 - (uint8_t) (intensitySlope * distanceToCenter);
                                    for (int iStripe = 0; iStripe < numStripes[iOrient][iColor]; ++iStripe) {
                                        for (auto&& pixels : pixelArray[stringIndex]) {
                                            pixels.raw[iColor] = intensity;
                                        }
                                        stringIndex = (stringIndex + stripeStep[iOrient][iColor]) % numStrings;
                                    }
                                }
                            }

                            break;

                        case horiz:

                            //logger.logMsg(LOG_DEBUG, name
                            //    + " horiz:  iColor=" + to_string(iColor)
                            //    + ",  widthPos[iColor]=" + to_string(widthPos[iColor])
                            //    + ",  sidebandWidth=" + to_string(sidebandWidth)
                            //    + ", intensitySlope=" + to_string(intensitySlope)
                            //    + ", stripeVPos[iOrient][iColor]=" + to_string(stripeVPos[iOrient][iColor])
                            //    + ", lowVEl=" + to_string(lowVEl)
                            //    + ", highVEl=" + to_string(highVEl));

                            for (int i = lowVEl; i <= highVEl; ++i) {
                                unsigned int vPixelIndex = ((i + vPixelsPerString) % vPixelsPerString);
                                // If this virtual pixel corresponds to a physical
                                // pixel, we'll draw into the physical pixels.
                                if (vPixelIndex % verticalVPixelRatio == 0) {
                                    unsigned int pixelIndex = vPixelIndex / verticalVPixelRatio;
                                    float distanceToCenter = abs(i - stripeVPos[iOrient][iColor]);
                                    uint8_t intensity = 255 - (uint8_t) (intensitySlope * distanceToCenter);
                                    for (int iStripe = 0; iStripe < numStripes[iOrient][iColor]; ++iStripe) {
                                        for (auto&& stringPixels : pixelArray) {
                                            stringPixels[pixelIndex].raw[iColor] = intensity;
                                        }
                                        pixelIndex = (pixelIndex + stripeStep[iOrient][iColor]) % pixelsPerString;
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

