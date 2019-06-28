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

// FillAndBurstPattern is based on RainbowExplosionPattern written by David Van Arnem in 2016.

#include "ConfigReader.h"
#include "FillAndBurstPattern.h"
#include "illumiconePixelUtility.h"
#include "illumiconeUtility.h"
#include "Log.h"
#include "Pattern.h"
#include "Widget.h"
#include "WidgetChannel.h"

using namespace std;


extern Log logger;

FillAndBurstPattern::FillAndBurstPattern(const std::string& name)
    : Pattern(name)
{
}


bool FillAndBurstPattern::initPattern(std::map<WidgetId, Widget*>& widgets)
{
    // ----- get pattern configuration -----

    string errMsgSuffix = " in " + name + " pattern configuration.";

    if (!patternConfigObject["burstingPriority"].is_number()) {
        logger.logMsg(LOG_ERR, "burstingPriority not specified" + errMsgSuffix);
        return false;
    }
    burstingPriority = patternConfigObject["burstingPriority "].int_value();
    logger.logMsg(LOG_INFO, name + " burstingPriority=" + to_string(burstingPriority));

    if (!ConfigReader::getIntValue(patternConfigObject, "lowPressureCutoff", lowPressureCutoff, errMsgSuffix, 1, 1023)) {
        return false;
    }
    logger.logMsg(LOG_INFO, name + " lowPressureCutoff=" + to_string(lowPressureCutoff));

    if (!ConfigReader::getIntValue(patternConfigObject, "burstThreshold", burstThreshold, errMsgSuffix, lowPressureCutoff, 1023)) {
        return false;
    }
    logger.logMsg(LOG_INFO, name + " burstThreshold=" + to_string(burstThreshold));

    if (!ConfigReader::getIntValue(patternConfigObject, "flashThreshold", flashThreshold, errMsgSuffix, burstThreshold, 1023)) {
        return false;
    }
    logger.logMsg(LOG_INFO, name + " flashThreshold=" + to_string(flashThreshold));

    string rgbStr;

    if (!ConfigReader::getStringValue(patternConfigObject, "pressurizationColor", rgbStr, errMsgSuffix)) {
        return false;
    }
    if (!stringToRgbPixel(rgbStr, pressurizationColor)) {
        logger.logMsg(LOG_ERR, "pressurizationColor value \"" + rgbStr + "\" is not valid" + errMsgSuffix);
        return false;
    }
    logger.logMsg(LOG_INFO, name + " pressurizationColor=" + rgbStr);

    if (ConfigReader::getStringValue(patternConfigObject, "depressurizationColor", rgbStr)) {
        if (!stringToRgbPixel(rgbStr, depressurizationColor)) {
            logger.logMsg(LOG_ERR, "depressurizationColor value \"" + rgbStr + "\" is not valid" + errMsgSuffix);
            return false;
        }
        displayDepressurization = true;
        logger.logMsg(LOG_INFO, name + " depressurizationColor=" + rgbStr);
    }
    else {
        displayDepressurization = false;
        logger.logMsg(LOG_INFO, name + " depressurization will not be displayed.");
    }

    if (!ConfigReader::getIntValue(patternConfigObject, "fillStepSize", fillStepSize, errMsgSuffix)) {
        return false;
    }
    logger.logMsg(LOG_INFO, name + " fillStepSize=" + to_string(fillStepSize));

    if (!ConfigReader::getIntValue(patternConfigObject, "fillStepIntervalHighMs", fillStepIntervalHighMs, errMsgSuffix)) {
        return false;
    }
    logger.logMsg(LOG_INFO, name + " fillStepIntervalHighMs=" + to_string(fillStepIntervalHighMs));

    if (!ConfigReader::getIntValue(patternConfigObject, "fillStepIntervalLowMs", fillStepIntervalLowMs, errMsgSuffix)) {
        return false;
    }
    logger.logMsg(LOG_INFO, name + " fillStepIntervalLowMs=" + to_string(fillStepIntervalLowMs));

    if (!ConfigReader::getIntValue(patternConfigObject, "flashIntervalMs", flashIntervalMs, errMsgSuffix)) {
        return false;
    }
    logger.logMsg(LOG_INFO, name + " flashIntervalMs=" + to_string(flashIntervalMs));

    if (!ConfigReader::getIntValue(patternConfigObject, "burstDurationMs", burstDurationMs, errMsgSuffix)) {
        return false;
    }
    logger.logMsg(LOG_INFO, name + " burstDurationMs=" + to_string(burstDurationMs));

    if (!ConfigReader::getIntValue(patternConfigObject, "flashDurationMs", flashDurationMs, errMsgSuffix)) {
        return false;
    }
    logger.logMsg(LOG_INFO, name + " flashDurationMs=" + to_string(flashDurationMs));

    std::vector<Pattern::ChannelConfiguration> channelConfigs = getChannelConfigurations(widgets);
    if (channelConfigs.empty()) {
        logger.logMsg(LOG_WARNING, "No valid widget channels are configured for " + name + ".");
        return false;
    }

    for (auto&& channelConfig : channelConfigs) {

        if (channelConfig.inputName == "pressure") {
            pressureChannel = channelConfig.widgetChannel;
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

    // The plain priority, which is retrieved from the config by the parent
    // class, is the priority used while filling (which is most of the time).
    fillingPriority = priority;

    state = PatternState::pressurizing;

    fillStepIntervalMeasmtRange = flashThreshold - burstThreshold;
    fillStepIntervalRange = fillStepIntervalHighMs - fillStepIntervalLowMs;

    return true;
}


void FillAndBurstPattern::clearAllPixels()
{
    for (auto&& pixels:pixelArray) {
        for (auto&& pixel:pixels) {
            pixel = CRGB::Black;
        }
    }
}


bool FillAndBurstPattern::update()
{
    // Don't do anything if no input channel was assigned.
    if (pressureChannel == nullptr) {
        return false;
    }

    unsigned int nowMs = getNowMs();

    bool gotNewMeasmt = false;
    int curMeasmt = 0;
    bool wasActive = isActive;
    isActive = pressureChannel->getIsActive();
    if (pressureChannel->getHasNewPositionMeasurement()) {
        curMeasmt = pressureChannel->getPosition();
        gotNewMeasmt = true;

        // Being below the low pressure cutoff is the same as being inactive.
        // TODO:  implement hysteresis
        if (curMeasmt <= lowPressureCutoff) {
            if (wasActive && isActive) {
                logger.logMsg(LOG_INFO, name + ":  Forcing inactive due to pressure at or below low-pressure cutoff.");
            }
            isActive = false;
        }
    }

    // Are we in one of the bursting states? 
    if (state == PatternState::fillRed
        || state == PatternState::fillOrange
        || state == PatternState::fillYellow
        || state == PatternState::fillGreen
        || state == PatternState::fillBlue
        || state == PatternState::fillIndigo
        || state == PatternState::fillViolet)
    {
        if (isActive && gotNewMeasmt) {
            if (curMeasmt > flashThreshold) {
                logger.logMsg(LOG_INFO, name + ":  Flashing.");
                state = PatternState::startFlashing;
            }
            else {
                // Map the pressure measurement into the fill step interval millisecond range
                // so that the bursting gets more furious as they keep pumping.
                fillStepIntervalMs =
                    fillStepIntervalHighMs
                     - (fillStepIntervalRange * (curMeasmt - burstThreshold) / fillStepIntervalMeasmtRange);
                fillStepIntervalMs = std::min(fillStepIntervalMs, fillStepIntervalHighMs);
                fillStepIntervalMs = std::max(fillStepIntervalMs, fillStepIntervalLowMs);
            }
        }
        // If it isn't time to do the next step, just return that we're active.
        if ((int) (nowMs - nextStepMs) < 0) {
            return true;
        }
    }

    switch (state) {

        case PatternState::pressurizing:

            if (!gotNewMeasmt || !isActive) {
                return isActive;
            }
  
            clearAllPixels();

            // Start bursting if we're past the maximum pressure.
            if (curMeasmt > burstThreshold) {
                logger.logMsg(LOG_INFO, name + ":  Bursting.");
                endBurstingMs = nowMs + burstDurationMs;
                fillPosition = pixelsPerString;
                fillStepIntervalMs = fillStepIntervalHighMs;
                nextStepMs = nowMs;         // immediately
                state = PatternState::fillRed;
            }
            else {
                // Fill the cone from the bottom up to represent the current pressure.
                // TODO:  use MeasurementMapper
                int fillLevel = pixelsPerString * (curMeasmt - lowPressureCutoff) / (burstThreshold - lowPressureCutoff);
                for (auto&& pixels:pixelArray) {
                    for (unsigned int i = pixelsPerString - fillLevel; i < pixelsPerString; i++) {
                        pixels[i] = pressurizationColor;
                    }
                }
            }

            break;

        case PatternState::fillRed:
            priority = burstingPriority;
            fillPosition = max(fillPosition - fillStepSize, 0);
            for (unsigned int i = fillPosition; i < pixelsPerString; i++) {
                for (auto&& pixels:pixelArray) {
                    pixels[i] = CRGB::Red;
                }
            }
            nextStepMs = nowMs + fillStepIntervalMs;
            if (fillPosition <= 0) {
                fillPosition = pixelsPerString;
                state = PatternState::fillOrange;
            }
            break;

        case PatternState::fillOrange:
            fillPosition = max(fillPosition - fillStepSize, 0);
            for (unsigned int i = fillPosition; i < pixelsPerString; i++) {
                for (auto&& pixels:pixelArray) {
                    pixels[i] = CRGB::Orange;
                }
            }
            nextStepMs = nowMs + fillStepIntervalMs;
            if (fillPosition <= 0) {
                fillPosition = pixelsPerString;
                state = PatternState::fillYellow;
            }
            break;

        case PatternState::fillYellow:
            fillPosition = max(fillPosition - fillStepSize, 0);
            for (unsigned int i = fillPosition; i < pixelsPerString; i++) {
                for (auto&& pixels:pixelArray) {
                    pixels[i] = CRGB::Yellow;
                }
            }
            nextStepMs = nowMs + fillStepIntervalMs;
            if (fillPosition <= 0) {
                fillPosition = pixelsPerString;
                state = PatternState::fillGreen;
            }
            break;

        case PatternState::fillGreen:
            fillPosition = max(fillPosition - fillStepSize, 0);
            for (unsigned int i = fillPosition; i < pixelsPerString; i++) {
                for (auto&& pixels:pixelArray) {
                    pixels[i] = CRGB::Green;
                }
            }
            nextStepMs = nowMs + fillStepIntervalMs;
            if (fillPosition <= 0) {
                fillPosition = pixelsPerString;
                state = PatternState::fillBlue;
            }
            break;

        case PatternState::fillBlue:
            fillPosition = max(fillPosition - fillStepSize, 0);
            for (unsigned int i = fillPosition; i < pixelsPerString; i++) {
                for (auto&& pixels:pixelArray) {
                    pixels[i] = CRGB::Blue;
                }
            }
            nextStepMs = nowMs + fillStepIntervalMs;
            if (fillPosition <= 0) {
                fillPosition = pixelsPerString;
                state = PatternState::fillIndigo;
            }
            break;

        case PatternState::fillIndigo:
            fillPosition = max(fillPosition - fillStepSize, 0);
            for (unsigned int i = fillPosition; i < pixelsPerString; i++) {
                for (auto&& pixels:pixelArray) {
                    pixels[i] = CRGB::Indigo;
                }
            }
            nextStepMs = nowMs + fillStepIntervalMs;
            if (fillPosition <= 0) {
                fillPosition = pixelsPerString;
                state = PatternState::fillViolet;
            }
            break;

        case PatternState::fillViolet:
            fillPosition = max(fillPosition - fillStepSize, 0);
            for (unsigned int i = fillPosition; i < pixelsPerString; i++) {
                for (auto&& pixels:pixelArray) {
                    pixels[i] = CRGB::Violet;
                }
            }
            nextStepMs = nowMs + fillStepIntervalMs;
            if (fillPosition <= 0) {
                // If they stopped pumping and the pressure has dropped below the burst threshold, we'll
                // stop bursting.  Otherwise, we'll keep bursting until the burst time limit is reached.
                if ((gotNewMeasmt && curMeasmt <= burstThreshold) || (int) (nowMs - endBurstingMs) >= 0) {
                    logger.logMsg(LOG_INFO, name + ":  Depressurizing.");
                    priority = fillingPriority;
                    clearAllPixels();
                    state = PatternState::depressurizing;
                }
                else {
                    state = PatternState::fillRed;
                }
            }
            break;

        case PatternState::startFlashing:
            endFlashingMs = nowMs + flashDurationMs;
            nextStepMs = nowMs;
            state = PatternState::flashOn;
            break;

        case PatternState::flashOn:
            if ((int) (nowMs - nextStepMs) >= 0) {
                fillSolid(pixelArray, RgbPixel::White);
                nextStepMs = nowMs + flashIntervalMs;
                state = PatternState::flashOff;
            }
            break;

        case PatternState::flashOff:
            if ((int) (nowMs - nextStepMs) >= 0) {
                clearAllPixels();
                if ((int) (nowMs - endFlashingMs) < 0) {
                    nextStepMs = nowMs + flashIntervalMs;
                    state = PatternState::flashOn;
                }
                else {
                    logger.logMsg(LOG_INFO, name + ":  Depressurizing after flashing.");
                    priority = fillingPriority;
                    clearAllPixels();
                    state = PatternState::depressurizing;
                }
            }
            break;

        case PatternState::depressurizing:
            // If the widget has gone inactive, we'll assume that
            // the pressure is below the low pressure cutoff.
            if (!isActive) {
                logger.logMsg(LOG_INFO, name + ":  Ending depressurization due to becoming inactive.  Now pressurizing.");
                state = PatternState::pressurizing;
            }
            else {
                // We'll wait for the pressure to drop to or below the
                // low pressure cutoff before allowing pressurization again.
                if (gotNewMeasmt) {
                    if (curMeasmt <= lowPressureCutoff) {
                        logger.logMsg(LOG_INFO, name + ":  Low-pressure reset reached.  Now pressurizing.");
                        isActive = false;
                        state = PatternState::pressurizing;
                    }
                    else {
                        if (displayDepressurization) {
                            // Fill the cone from the bottom up to represent the current pressure.
                            clearAllPixels();
                            // TODO:  use MeasurementMapper
                            int fillLevel = curMeasmt >= burstThreshold
                                          ? pixelsPerString
                                          : pixelsPerString * (curMeasmt - lowPressureCutoff) / (burstThreshold - lowPressureCutoff);
                            for (auto&& pixels:pixelArray) {
                                for (unsigned int i = pixelsPerString - fillLevel; i < pixelsPerString; i++) {
                                    pixels[i] = depressurizationColor;
                                }
                            }
                        }
                        else {
                            isActive = false;
                        }
                    }
                }
            }
            break;
    }

    return isActive;
}
