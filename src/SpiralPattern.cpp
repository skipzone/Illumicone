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

        if (channelConfig.inputName == "compression") {
            compressionChannel = channelConfig.widgetChannel;
        }
        else if (channelConfig.inputName == "rotation") {
            rotationChannel = channelConfig.widgetChannel;
        }
        else if (channelConfig.inputName == "color") {
            colorChannel = channelConfig.widgetChannel;
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

    // -- spiral --

    if (!ConfigReader::getBoolValue(patternConfig, "flipSpring", flipSpring, errMsgSuffix)) {
        return false;
    }
    logMsg(LOG_INFO, name + " flipSpring=" + to_string(flipSpring));

    if (!ConfigReader::getFloatValue(patternConfig, "spiralTightnessFactor", spiralTightnessFactor, errMsgSuffix)) {
        return false;
    }
    logMsg(LOG_INFO, name + " spiralTightnessFactor=" + to_string(spiralTightnessFactor));

    if (!ConfigReader::getFloatValue(patternConfig, "progressiveSpringFactor", progressiveSpringFactor, errMsgSuffix)) {
        return false;
    }
    logMsg(LOG_INFO, name + " progressiveSpringFactor=" + to_string(progressiveSpringFactor));

    if (!ConfigReader::getFloatValue(patternConfig, "progressiveSpringCompressionResponseFactor", progressiveSpringCompressionResponseFactor, errMsgSuffix)) {
        return false;
    }
    logMsg(LOG_INFO, name + " progressiveSpringCompressionResponseFactor=" + to_string(progressiveSpringCompressionResponseFactor));

    // -- compression --

    if (compressionChannel != nullptr) {

        if (!compressionMeasmtMapper.readConfig(patternConfig, "compressionMeasurementMapper", errMsgSuffix)) {
            return false;
        }

        if (!ConfigReader::getIntValue(patternConfig, "compressionResetTimeoutSeconds", compressionResetTimeoutSeconds, errMsgSuffix)) {
            return false;
        }
        logMsg(LOG_INFO, name + " compressionResetTimeoutSeconds=" + to_string(compressionResetTimeoutSeconds));

        compressionTriangleAmplitude = maxCyclicalCompression - minCyclicalCompression;
        compressionTrianglePeriod = compressionTriangleAmplitude * 2;
    }


    // -- rotation --

    if (rotationChannel != nullptr) {
        if (!rotationMeasmtMapper.readConfig(patternConfig, "rotationMeasurementMapper", errMsgSuffix)) {
            return false;
        }
    }


    // -- color --

    if (colorChannel != nullptr) {
        if (!colorMeasmtMapper.readConfig(patternConfig, "colorMeasurementMapper", errMsgSuffix)) {
            return false;
        }
    }


    // ----- initialize object data -----

    nextResetCompressionMs = 0;
    resetCompression = true;
    //compressionPos = compressionDivisor;
    compressionFactor = 1;      // TODO: need to flag it as invalid until we actually get a good measurement
    rotationStepDelayMs = 0;
    rotationOffset = 0;
    currentHue = 0;

    return true;
}


bool SpiralPattern::update()
{
    isActive = false;
    bool gotUpdateFromWidget = false;
    unsigned int nowMs = getNowMs();

//    // Get any updated rotation.
//    if (rotationChannel != nullptr) {
//        if (rotationChannel->getIsActive()) {
//            isActive = true;
//            if (rotationChannel->getHasNewPositionMeasurement()) {
//                gotUpdateFromWidget = true;
//                rPos = ((unsigned int) rotationChannel->getPosition()) % pixelsPerString;
//            }
//        }
//    }

    // TODO:  The next two blocks do the same thing but with different data.
    //        Implement in one function.

    // Get an updated compression, if available.
    if (compressionChannel != nullptr) {
        if (compressionChannel->getIsActive()) {
            isActive = true;
            if (compressionChannel->getHasNewPositionMeasurement()) {
                int rawCompressionPos = compressionChannel->getPosition();
                if (compressionMeasmtMapper.mapMeasurement(rawCompressionPos, compressionFactor))
                    gotUpdateFromWidget = true;

                    if (resetCompression) {
                        resetCompression = false;
                        compressionPosOffset = rawCompressionPos;
                    }

                    //logMsg(LOG_DEBUG, name + ":  rawCompressionPos=" + to_string(rawCompressionPos)
                    //                  + ", compressionPosOffset=" + to_string(compressionPosOffset)
                    //                  + ", compressionFactor=" + to_string(compressionFactor));


                // TODO:  Put back in cyclical measurement support.
                /*
                compressionPos = (rawCompressionPos - compressionPosOffset) / compressionScaleFactor;
                if (maxCyclicalCompression != 0) {
                    // This is a triangle wave function where the height of the triangle (peak-to-peak
                    // amplitude of the waveform) is maxCyclicalCompression - minCyclicalCompression,
                    // and the width of the base (waveform period) is twice the height.   The waveform
                    // is offset from zero (DC offset) by minCyclicalCompression.  We left-shift the
                    // wave so that compressionPos starts out at minCyclicalCompression.
                    int singleCycleX = abs(compressionPos + compressionTriangleAmplitude) % compressionTrianglePeriod;
                    //logMsg(LOG_DEBUG, name + ":  compressionTriangleAmplitude=" + to_string(compressionTriangleAmplitude)
                    //                  + ", compressionTrianglePeriod=" + to_string(compressionTrianglePeriod)
                    //                  + ", singleCycleX=" + to_string(singleCycleX));
                    compressionPos = abs(singleCycleX - compressionTriangleAmplitude) + minCyclicalCompression;
                }
// TODO:  make sure that the cyclical stuff isn't dependent on this check
//                if (compressionPos < 1) {
//                    compressionPos = 1;
//                }
                logMsg(LOG_DEBUG, name + ":  rawCompressionPos=" + to_string(rawCompressionPos)
                                  + ", compressionPosOffset=" + to_string(compressionPosOffset)
                                  + ", compressionPos=" + to_string(compressionPos));
                */

            }
        }
    }

    // Get an updated color, if available.
    if (colorChannel != nullptr) {
        if (colorChannel->getIsActive()) {
            isActive = true;
            if (colorChannel->getHasNewPositionMeasurement()) {
                int rawColorPos = colorChannel->getPosition();
                logMsg(LOG_DEBUG, "new color measurement " + to_string(rawColorPos));
                int colorSteps;
                if (colorMeasmtMapper.mapMeasurement(rawColorPos, colorSteps)) {
                    gotUpdateFromWidget = true;
                    currentHue += colorSteps;
                    logMsg(LOG_DEBUG, name + ":  rawColorPos=" + to_string(rawColorPos)
                                      + ", colorSteps=" + to_string(colorSteps)
                                      + ", currentHue=" + to_string(currentHue));
                }
            }
        }
    }


    // Set flag to force compression back to 1 when compression channel
    // has been inactive for compressionResetTimeoutSeconds.
    if (isActive) {
        nextResetCompressionMs = 0;
    }
    else {
        if (nextResetCompressionMs == 0) {
            nextResetCompressionMs = nowMs + compressionResetTimeoutSeconds * 1000;
        }
        else if (!resetCompression && (int) (nowMs - nextResetCompressionMs) >= 0) {
            //logMsg(LOG_DEBUG, name + ":  Resetting compression.");
            resetCompression = true;
            compressionPos = 1;
        }
    }


    // Draw.

    if (gotUpdateFromWidget) {

        clearAllPixels(pixelArray);

        CHSV hsvCurrentColor((uint8_t) currentHue, 255, 255);
        CRGB rgbCurrentColor;
        hsv2rgb(hsvCurrentColor, rgbCurrentColor);

        unsigned int heightInPixels = (1.0 / compressionFactor) * (float) pixelsPerString;
        unsigned int startingPixelOffset = pixelsPerString - heightInPixels;

        // Range of y is [0..1].
        float xStep = 1.0 / numStrings;
        float x = 0;
        for (unsigned int i = 0; i < numStrings * heightInPixels; ++i) {

            float y = ::powf(x / spiralTightnessFactor,
                                progressiveSpringFactor + progressiveSpringCompressionResponseFactor * compressionFactor);
            if (y > 1.0) {
                break;
            }

            unsigned int stringIdx = (unsigned int) (x / xStep) % numStrings;
            unsigned int pixelIdx = (flipSpring ? (1.0 - y) : y) * heightInPixels + startingPixelOffset;
            //logMsg(LOG_DEBUG, name + ":  x=" + to_string(x) + " y=" + to_string(y) + " pixelIdx=" + to_string(pixelIdx));
            // When the spiral is stretched, make sure we're
            // setting a pixel within the physical bounds.
            if (pixelIdx < pixelsPerString) {
                // TODO:  set the color
                pixelArray[stringIdx][pixelIdx] = rgbCurrentColor;
            }

            x += xStep;
        }

    }

    return isActive;
}

