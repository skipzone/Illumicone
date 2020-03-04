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
#include "StripePattern.h"
#include "Widget.h"
#include "WidgetChannel.h"

using namespace std;


extern Log logger;


StripePattern::StripePattern(const std::string& name)
    : Pattern(name, true)   // usesHsvModel = true
    , isHorizontal(false)
    , virtualPixelRatio(1)
    , numPixelsInDrawingPlane(numStrings)
    , numVirtualPixelsInDrawingPlane(numStrings)
{
}


bool StripePattern::initPattern(std::map<WidgetId, Widget*>& widgets)
{
    // ----- get input channels -----

    std::vector<Pattern::ChannelConfiguration> channelConfigs = getChannelConfigurations(widgets);
    if (channelConfigs.empty()) {
        logger.logMsg(LOG_ERR, "No valid widget channels are configured for " + name + ".");
        return false;
    }

    for (auto&& channelConfig : channelConfigs) {
        bool recognizedChannel = false;

        if (channelConfig.inputName == "position") {
            positionChannel = channelConfig.widgetChannel;
            recognizedChannel = true;
        }
        else if (channelConfig.inputName == "width") {
            widthChannel = channelConfig.widgetChannel;
            recognizedChannel = true;
        }
        else if (channelConfig.inputName == "hue") {
            hueChannel = channelConfig.widgetChannel;
            recognizedChannel = true;
        }
        else if (channelConfig.inputName == "saturation") {
            saturationChannel = channelConfig.widgetChannel;
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


    string errMsgSuffix = " in " + name + " pattern configuration.";


    // ----- measurement mapper configuration -----

    if (positionChannel != nullptr) {
        if (!positionMeasmtMapper.readConfig(patternConfigObject, "positionMeasurementMapper", errMsgSuffix)) {
            return false;
        }
    }

    if (widthChannel != nullptr) {
        if (!widthMeasmtMapper.readConfig(patternConfigObject, "widthMeasurementMapper", errMsgSuffix)) {
            return false;
        }
    }

    if (hueChannel != nullptr) {
        if (!hueMeasmtMapper.readConfig(patternConfigObject, "hueMeasurementMapper", errMsgSuffix)) {
            return false;
        }
    }

    if (saturationChannel != nullptr) {
        if (!saturationMeasmtMapper.readConfig(patternConfigObject, "saturationMeasurementMapper", errMsgSuffix)) {
            return false;
        }
    }


    // ----- layout configuration -----

    if (!ConfigReader::getBoolValue(patternConfigObject, "isHorizontal", isHorizontal, errMsgSuffix)) {
        return false;
    }
    logger.logMsg(LOG_INFO, "%s isHorizontal=%d", name.c_str(), isHorizontal);

    if (!ConfigReader::getUnsignedIntValue(patternConfigObject, "virtualPixelRatio", virtualPixelRatio, errMsgSuffix, 1)) {
        return false;
    }
    logger.logMsg(LOG_INFO, "%s virtualPixelRatio=%d", name.c_str(), virtualPixelRatio);

    if (isHorizontal) {
        numPixelsInDrawingPlane = pixelsPerString;
        numVirtualPixelsInDrawingPlane = pixelsPerString * virtualPixelRatio;
    }
    else {
        numPixelsInDrawingPlane = numStrings;
        numVirtualPixelsInDrawingPlane = numStrings * virtualPixelRatio;
    }

    if (!ConfigReader::getIntValue( patternConfigObject, "numStripes", numStripes, errMsgSuffix, 1, numStrings / 2)) {
        return false;
    }
    if (numPixelsInDrawingPlane % numStripes != 0) {
        logger.logMsg(LOG_WARNING, "numStripes in %s pattern configuration should be a factor of %d.",
                      name.c_str(), numPixelsInDrawingPlane);
    }
    logger.logMsg(LOG_INFO, "%s numStripes=%d", name.c_str(), numStripes);
    stripeStep = numPixelsInDrawingPlane / numStripes;

    if (!ConfigReader::getIntValue(
        patternConfigObject, "minSidebandWidth", minSidebandWidth, errMsgSuffix,
        0, numVirtualPixelsInDrawingPlane / 2))
    {
        return false;
    }
    logger.logMsg(LOG_INFO, "%s minSidebandWidth=%d", name.c_str(), minSidebandWidth);

    if (!ConfigReader::getIntValue(
        patternConfigObject, "maxSidebandWidth", maxSidebandWidth, errMsgSuffix,
        minSidebandWidth, numVirtualPixelsInDrawingPlane / 2))
    {
        return false;
    }
    logger.logMsg(LOG_INFO, "%s maxSidebandWidth=%d", name.c_str(), maxSidebandWidth);


    // ----- hue configuration -----

    if (!ConfigReader::getFloatValue(patternConfigObject, "startingHue", startingHue, errMsgSuffix, 0, 255.0)) {
        return false;
    }
    logger.logMsg(LOG_INFO, name + " startingHue=" + to_string(startingHue));

    if (!ConfigReader::getFloatValue(patternConfigObject, "endingHue", endingHue, errMsgSuffix, 0, 255.0)) {
        return false;
    }
    logger.logMsg(LOG_INFO, name + " endingHue=" + to_string(endingHue));

    if (!ConfigReader::getBoolValue(patternConfigObject, "hueDirectionIsBlueToRed", hueDirectionIsBlueToRed)) {
        hueDirectionIsBlueToRed = (startingHue > endingHue);
    }
    logger.logMsg(LOG_INFO, "%s hueDirectionIsBlueToRed=%d", name.c_str(), isHorizontal);

    if (!ConfigReader::getFloatValue(patternConfigObject, "hueFoldbackPct", hueFoldbackPct, errMsgSuffix, 0, 99.9)) {
        return false;
    }
    logger.logMsg(LOG_INFO, name + " hueFoldbackPct=" + to_string(hueFoldbackPct));

    if (!ConfigReader::getUnsignedIntValue(
        patternConfigObject, "hueRepeat", hueRepeat, errMsgSuffix,
        1, numVirtualPixelsInDrawingPlane / 2))
    {
        return false;
    }
    logger.logMsg(LOG_INFO, name + " hueRepeat=" + to_string(hueRepeat));


    // ----- saturation configuration -----

    if (!ConfigReader::getFloatValue(patternConfigObject, "startingSaturation", startingSaturation, errMsgSuffix, 0, 255.0)) {
        return false;
    }
    logger.logMsg(LOG_INFO, name + " startingSaturation=" + to_string(startingSaturation));

    if (!ConfigReader::getFloatValue(patternConfigObject, "endingSaturation", endingSaturation, errMsgSuffix, 0, 255.0)) {
        return false;
    }
    logger.logMsg(LOG_INFO, name + " endingSaturation=" + to_string(endingSaturation));

    saturationDirectionIsDecreasing = (startingSaturation > endingSaturation);

    if (!ConfigReader::getFloatValue(patternConfigObject, "saturationFoldbackPct", saturationFoldbackPct, errMsgSuffix, 0, 99.9)) {
        return false;
    }
    logger.logMsg(LOG_INFO, name + " saturationFoldbackPct=" + to_string(saturationFoldbackPct));

    if (!ConfigReader::getUnsignedIntValue(
        patternConfigObject, "saturationRepeat", saturationRepeat, errMsgSuffix,
        1, numVirtualPixelsInDrawingPlane / 2))
    {
        return false;
    }
    logger.logMsg(LOG_INFO, name + " saturationRepeat=" + to_string(saturationRepeat));


    // TODO:  maybe specify separate sat and value for sideband gradient


    // ----- misc. configuration -----

    if (widthChannel != nullptr) {
        if (!ConfigReader::getIntValue(patternConfigObject, "widthResetTimeoutSeconds", widthResetTimeoutSeconds, errMsgSuffix, 1)) {
            return false;
        }
        logger.logMsg(LOG_INFO, name + " widthResetTimeoutSeconds=" + to_string(widthResetTimeoutSeconds));
    }

    // TODO:  replace stripeHsv with dynamic hue and saturation
    string hsvStr;
    if (!ConfigReader::getHsvPixelValue(patternConfigObject, "defaultHsv", hsvStr, stripeHsv, errMsgSuffix)) {
        return false;
    }
    logger.logMsg(LOG_INFO, name + " stripeHsv=" + hsvStr);

    // TODO:  replace scaledown factor with mapper
    if (!ConfigReader::getIntValue(patternConfigObject, "scaledownFactor", scaledownFactor, errMsgSuffix, 1)) {
        return false;
    }
    logger.logMsg(LOG_INFO, "%s scaledownFactor=%d", name.c_str(), scaledownFactor);

    // TODO:  replace widthScaledownFactor with mapper
    if (!ConfigReader::getIntValue( patternConfigObject, "widthScaledownFactor", widthScaledownFactor, errMsgSuffix, 1)) {
        return false;
    }
    logger.logMsg(LOG_INFO, "%s widthScaledownFactor=%d", name.c_str(), widthScaledownFactor);


    // ----- initialize object data -----

    stripeVirtualPos = 0;
    widthPos = minSidebandWidth;
    nextResetWidthMs = 0;
    resetWidth = true;

    return true;
}


bool StripePattern::update()
{
    isActive = false;
    bool gotPositionOrWidthUpdate = false;
    int rawPosition;
    unsigned int nowMs = getNowMs();


/*
        MeasurementMapper<int, float> hueMeasmtMapper;
        MeasurementMapper<int, float> saturationMeasmtMapper;
        MeasurementMapper<int, int> positionMeasmtMapper;
        MeasurementMapper<int, int> widthMeasmtMapper;

                if (rotationMeasmtMapper.mapMeasurement(rawRotationPos, rotationStepIntervalMs)) {
                    gotUpdateFromWidget = true;
                    rotateCounterclockwise = (rawRotationPos >= 0);
                    //logger.logMsg(LOG_DEBUG, name + ":  rawRotationPos=" + to_string(rawRotationPos)
                    //                  + ", rotateCounterclockwise=" + to_string(rotateCounterclockwise)
                    //                  + ", rotationStepIntervalMs=" + to_string(rotationStepIntervalMs)
                    //                  + ", rotationOffset=" + to_string(rotationOffset));
                }
*/


    if (positionChannel != nullptr) {
        if (positionChannel->getIsActive()) {
            isActive = true;
            if (positionChannel->getHasNewPositionMeasurement()) {
                gotPositionOrWidthUpdate = true;
                rawPosition = positionChannel->getPosition();
                stripeVirtualPos = ((unsigned int) rawPosition) / scaledownFactor % numVirtualPixelsInDrawingPlane;
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
                    // Remember what position now corresponds to the minimum width.
                    widthPosOffset = rawWidthPos;
                }

                if (maxSidebandWidth > 0) {
                    int scaledWidthPos = (rawWidthPos - widthPosOffset) / widthScaledownFactor;
                    // TODO:  This doesn't work as expected.  widthPos can end up greater than maxSidebandWidth.
                    // This is a triangle wave function where the period is maxSidebandWidth * 2
                    // and the range is minSidebandWidth to maxSidebandWidth.  We right-shift the
                    // wave so that the width starts out at minSidebandWidth.
                    widthPos =
                        abs(abs(scaledWidthPos + maxSidebandWidth) % (maxSidebandWidth * 2) - maxSidebandWidth)
                        + minSidebandWidth;
                    //logger.logMsg(LOG_DEBUG, name
                    //              + ", rawWidthPos=" + to_string(rawWidthPos)
                    //              + ", widthPosOffset=" + to_string(widthPosOffset)
                    //              + ", scaledWidthPos=" + to_string(scaledWidthPos)
                    //              + ", widthPos=" + to_string(widthPos));
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
            widthPos = minSidebandWidth;
        }
    }

    // Draw the stripes.
    if (gotPositionOrWidthUpdate) {

        clearAllPixels(coneStrings);

        int sidebandWidth = widthPos;
        float valueSlope = (float) stripeHsv.value / (float) (sidebandWidth + 1);

        // TODO:  rename these
        int lowVEl = stripeVirtualPos - sidebandWidth;
        int highVEl = stripeVirtualPos + sidebandWidth;

        if (isHorizontal) {

            //logger.logMsg(LOG_DEBUG, name
            //    + ",  widthPos=" + to_string(widthPos)
            //    + ",  sidebandWidth=" + to_string(sidebandWidth)
            //    + ", valueSlope=" + to_string(valueSlope)
            //    + ", stripeVirtualPos=" + to_string(stripeVirtualPos)
            //    + ", lowVEl=" + to_string(lowVEl)
            //    + ", highVEl=" + to_string(highVEl));

            for (int i = lowVEl; i <= highVEl; ++i) {
                unsigned int virtualPixelIdx = ((i + numVirtualPixelsInDrawingPlane) % numVirtualPixelsInDrawingPlane);
                // If this virtual pixel corresponds to a physical
                // pixel, we'll draw into the physical pixels.
                if (virtualPixelIdx % virtualPixelRatio == 0) {
                    unsigned int pixelIndex = virtualPixelIdx / virtualPixelRatio;
                    float distanceToCenter = abs(i - stripeVirtualPos);
                    uint8_t value = stripeHsv.value - (uint8_t) (valueSlope * distanceToCenter);
                    for (int iStripe = 0; iStripe < numStripes; ++iStripe) {
                        for (auto&& coneString : coneStrings) {
                            coneString[pixelIndex] = stripeHsv;
                            coneString[pixelIndex].value = value;
                        }
                        pixelIndex = (pixelIndex + stripeStep) % pixelsPerString;
                    }
                }
            }

        }
        else {

            //logger.logMsg(LOG_DEBUG, name
            //    + ",  widthPos=" + to_string(widthPos)
            //    + ",  sidebandWidth=" + to_string(sidebandWidth)
            //    + ", valueSlope=" + to_string(valueSlope)
            //    + ", stripeVirtualPos=" + to_string(stripeVirtualPos)
            //    + ", lowVEl=" + to_string(lowVEl)
            //    + ", highVEl=" + to_string(highVEl));

            for (int i = lowVEl; i <= highVEl; ++i) {
                unsigned int virtualStringIdx = ((i + numVirtualPixelsInDrawingPlane) % numVirtualPixelsInDrawingPlane);
                // If this virtual string corresponds to a physical
                // string, we'll draw into the physical string.
                if (virtualStringIdx % virtualPixelRatio == 0) {
                    unsigned int stringIndex = virtualStringIdx / virtualPixelRatio;
                    float distanceToCenter = abs(i - stripeVirtualPos);
                    uint8_t value = stripeHsv.value - (uint8_t) (valueSlope * distanceToCenter);
                    for (int iStripe = 0; iStripe < numStripes; ++iStripe) {
                        for (auto&& pixel : coneStrings[stringIndex]) {
                            pixel = stripeHsv;
                            pixel.value = value;
                        }
                        stringIndex = (stringIndex + stripeStep) % numStrings;
                    }
                }
            }
        }
    }

    return isActive;
}

