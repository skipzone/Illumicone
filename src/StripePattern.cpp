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

    if (!ConfigReader::getFloatValue(patternConfigObject, "hueFoldbackPct", hueFoldbackPct, errMsgSuffix, 0, 99.9)) {
        return false;
    }
    logger.logMsg(LOG_INFO, name + " hueFoldbackPct=" + to_string(hueFoldbackPct));

    if (!ConfigReader::getFloatValue(
        patternConfigObject, "hueRepeat", hueRepeat, errMsgSuffix,
        1, numVirtualPixelsInDrawingPlane / 2))
    {
        return false;
    }
    logger.logMsg(LOG_INFO, name + " hueRepeat=" + to_string(hueRepeat));

    hueDirectionIsRedToBlue = (startingHue < endingHue);
    hueSpan = fabsf(endingHue - startingHue);
    hueStep = (hueSpan * (float) hueRepeat) / (float) (isHorizontal ? numStrings : pixelsPerString);


    // ----- saturation configuration -----

    if (!ConfigReader::getFloatValue(patternConfigObject, "startingSaturation", startingSaturation, errMsgSuffix, 0, 255.0)) {
        return false;
    }
    logger.logMsg(LOG_INFO, name + " startingSaturation=" + to_string(startingSaturation));

    if (!ConfigReader::getFloatValue(patternConfigObject, "endingSaturation", endingSaturation, errMsgSuffix, 0, 255.0)) {
        return false;
    }
    logger.logMsg(LOG_INFO, name + " endingSaturation=" + to_string(endingSaturation));

    if (!ConfigReader::getFloatValue(patternConfigObject, "saturationFoldbackPct", saturationFoldbackPct, errMsgSuffix, 0, 99.9)) {
        return false;
    }
    logger.logMsg(LOG_INFO, name + " saturationFoldbackPct=" + to_string(saturationFoldbackPct));

    if (!ConfigReader::getFloatValue(
        patternConfigObject, "saturationRepeat", saturationRepeat, errMsgSuffix,
        1, numVirtualPixelsInDrawingPlane / 2))
    {
        return false;
    }
    logger.logMsg(LOG_INFO, name + " saturationRepeat=" + to_string(saturationRepeat));

    saturationDirectionIsDecreasing = (startingSaturation > endingSaturation);
    saturationSpan = fabsf(endingSaturation - startingSaturation);
    saturationStep = (saturationSpan * (float) saturationRepeat) / (float) (isHorizontal ? numStrings : pixelsPerString);


    // TODO:  maybe specify separate sat and value for sideband gradient


    // ----- misc. configuration -----

    if (widthChannel != nullptr) {
        if (!ConfigReader::getIntValue(
            patternConfigObject, "widthResetTimeoutSeconds", widthResetTimeoutSeconds, errMsgSuffix, 1))
        {
            return false;
        }
        logger.logMsg(LOG_INFO, name + " widthResetTimeoutSeconds=" + to_string(widthResetTimeoutSeconds));
    }

    if (!ConfigReader::getIntValue(patternConfigObject, "stripeCenterValue", stripeCenterValue, errMsgSuffix, 1)) {
        return false;
    }
    logger.logMsg(LOG_INFO, "%s stripeCenterValue=%d", name.c_str(), stripeCenterValue);


    // ----- measurement mapper configuration -----

    // TODO:  implement a way to verify that the min and max outputs from a mapper are within range

    //        (e.g., position output is [0, numVirtualPixelsInDrawingPlane)
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


    // ----- initialize object data -----

    stripeVirtualPos = 0;
    widthPos = minSidebandWidth;
    hueOffset = startingHue;
    saturationOffset = startingSaturation;
    nextResetWidthMs = 0;
    resetWidth = true;

    return true;
}


void StripePattern::setPixel(unsigned int stringIdx, unsigned int pixelIdx, float& hue, float& sat, uint8_t val)
{
    // TODO:  implement hue and saturation foldback

    coneStrings[stringIdx][pixelIdx].h = hue;
    coneStrings[stringIdx][pixelIdx].s = sat;
    coneStrings[stringIdx][pixelIdx].v = val;

    if (hueDirectionIsRedToBlue) {
        hue += hueStep;
        if (hue > endingHue) {
            hue -= hueSpan;
        }
    }
    else {
        hue -= hueStep;
        if (hue < endingHue) {
            hue += hueSpan;
        }
    }

    if (saturationDirectionIsDecreasing) {
        sat -= saturationStep;
        if (sat < endingSaturation) {
            sat += saturationSpan;
        }
    }
    else {
        sat += saturationStep;
        if (sat > endingSaturation) {
            sat -= saturationSpan;
        }
    }

}


bool StripePattern::update()
{
    isActive = false;
    bool gotPositionOrWidthUpdate = false;
    unsigned int nowMs = getNowMs();

    // Get an updated stripe position, if available.
    if (positionChannel != nullptr) {
        if (positionChannel->getIsActive()) {
            isActive = true;
            if (positionChannel->getHasNewPositionMeasurement()) {
                int rawPosition = positionChannel->getPosition();
                if (positionMeasmtMapper.mapMeasurement(rawPosition, stripeVirtualPos)) {
                    gotPositionOrWidthUpdate = true;
                    // Make sure stripeVirtualPos is in range even if the mapper destination range is misconfigured.
                    stripeVirtualPos = stripeVirtualPos % numVirtualPixelsInDrawingPlane;
                    //logger.logMsg(LOG_DEBUG, name + ":  rawPosition=" + to_string(rawPosition)
                    //                  + ", stripeVirtualPos=" + to_string(stripeVirtualPos));
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
                    // Remember what position now corresponds to the minimum width.
                    widthPosOffset = rawWidthPos;
                }

                if (maxSidebandWidth > 0) {
                    int scaledWidthPos;
                    if (widthMeasmtMapper.mapMeasurement(rawWidthPos, scaledWidthPos)) {
                        // TODO:  make sure mapper output is in range
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

    // Get an updated hue offset, if available.
    if (hueChannel != nullptr) {
        if (hueChannel->getIsActive()) {
            isActive = true;
            if (hueChannel->getHasNewPositionMeasurement()) {
                int rawPosition = hueChannel->getPosition();
                if (hueMeasmtMapper.mapMeasurement(rawPosition, hueOffset)) {
                    gotPositionOrWidthUpdate = true;
                }
            }
        }
    }

    // Get an updated saturation offset, if available.
    if (saturationChannel != nullptr) {
        if (saturationChannel->getIsActive()) {
            isActive = true;
            if (saturationChannel->getHasNewPositionMeasurement()) {
                int rawPosition = saturationChannel->getPosition();
                if (saturationMeasmtMapper.mapMeasurement(rawPosition, saturationOffset)) {
                    gotPositionOrWidthUpdate = true;
                }
            }
        }
    }

    // Draw the stripes.
    if (gotPositionOrWidthUpdate) {

        clearAllPixels(coneStrings);

        int sidebandWidth = widthPos;
        float valueSlope = (float) stripeCenterValue / (float) (sidebandWidth + 1);

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
                    unsigned int pixelIdx = virtualPixelIdx / virtualPixelRatio;
                    float distanceToCenter = abs(i - stripeVirtualPos);
                    uint8_t value = stripeCenterValue - (uint8_t) (valueSlope * distanceToCenter);
                    for (int iStripe = 0; iStripe < numStripes; ++iStripe) {

                        float hue = hueOffset;
                        float sat = saturationOffset;
                        //logger.logMsg(LOG_DEBUG, "%s i=%d pixelIdx=%d iStripe=%d hueSpan=%f hueStep=%f hue=%f",
                        //              name.c_str(), i, pixelIdx, iStripe, hueSpan hueStep, hue);

                        // TODO:  implement hue and saturation foldback

                        for (unsigned int stringIdx = 0; stringIdx < numStrings; ++stringIdx) {
                            setPixel(stringIdx, pixelIdx, hue, sat, value);
                        }

                        // Jump to next stripe.
                        pixelIdx = (pixelIdx + stripeStep) % pixelsPerString;
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
                    unsigned int stringIdx = virtualStringIdx / virtualPixelRatio;

                    float distanceToCenter = abs(i - stripeVirtualPos);
                    uint8_t value = stripeCenterValue - (uint8_t) (valueSlope * distanceToCenter);

                    for (int iStripe = 0; iStripe < numStripes; ++iStripe) {

                        float hue = hueOffset;
                        float sat = saturationOffset;

                        for (unsigned int pixelIdx = 0; pixelIdx < pixelsPerString; ++pixelIdx) {
                            setPixel(stringIdx, pixelIdx, hue, sat, value);
                        }

                        // Jump to next stripe.
                        stringIdx = (stringIdx + stripeStep) % numStrings;
                    }
                }
            }
        }
    }

    return isActive;
}

