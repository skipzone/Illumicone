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
#include "log.h"
#include "Pattern.h"
#include "Widget.h"
#include "WidgetChannel.h"


using namespace std;


FillAndBurstPattern::FillAndBurstPattern(const std::string& name)
    : Pattern(name)
{
}


bool FillAndBurstPattern::initPattern(ConfigReader& config, std::map<WidgetId, Widget*>& widgets)
{
    // The plain priority, which is retrieved from the config by the parent
    // class, is the priority used while filling (which is most of the time).
    fillingPriority = priority;

    state = PatternState::pressurizing;


    // ----- get pattern configuration -----

    string errMsgSuffix = " in " + name + " pattern configuration.";

    auto patternConfig = config.getPatternConfigJsonObject(name);

    if (!patternConfig["burstingPriority"].is_number()) {
        logMsg(LOG_ERR, "burstingPriority not specified" + errMsgSuffix);
        return false;
    }
    burstingPriority = patternConfig["burstingPriority "].int_value();
    logMsg(LOG_INFO, name + " burstingPriority=" + to_string(burstingPriority));

    if (!patternConfig["lowPressureCutoff"].is_number()) {
        logMsg(LOG_ERR, "lowPressureCutoff not specified" + errMsgSuffix);
        return false;
    }
    lowPressureCutoff = patternConfig["lowPressureCutoff"].int_value();
    logMsg(LOG_INFO, name + " lowPressureCutoff=" + to_string(lowPressureCutoff));
    if (lowPressureCutoff <= 0) {
        logMsg(LOG_ERR, "lowPressureCutoff is zero or less" + errMsgSuffix);
        return false;
    }

    if (!patternConfig["burstThreshold"].is_number()) {
        logMsg(LOG_ERR, "burstThreshold not specified" + errMsgSuffix);
        return false;
    }
    burstThreshold = patternConfig["burstThreshold"].int_value();
    logMsg(LOG_INFO, name + " burstThreshold=" + to_string(burstThreshold));
    if (burstThreshold <= 0) {
        logMsg(LOG_ERR, "burstThreshold is zero or less" + errMsgSuffix);
        return false;
    }

    if (lowPressureCutoff >= burstThreshold) {
        logMsg(LOG_ERR, "lowPressureCutoff must be lower than the burstThreshold" + errMsgSuffix);
        return false;
    }

    string rgbStr;

    if (!ConfigReader::getStringValue(patternConfig, "pressurizationColor", rgbStr, errMsgSuffix)) {
        return false;
    }
    if (!stringToRgbPixel(rgbStr, pressurizationColor)) {
        logMsg(LOG_ERR, "pressurizationColor value \"" + rgbStr + "\" is not valid" + errMsgSuffix);
        return false;
    }
    logMsg(LOG_INFO, name + " pressurizationColor=" + rgbStr);

    if (!ConfigReader::getStringValue(patternConfig, "depressurizationColor", rgbStr, errMsgSuffix)) {
        return false;
    }
    if (!stringToRgbPixel(rgbStr, depressurizationColor)) {
        logMsg(LOG_ERR, "depressurizationColor value \"" + rgbStr + "\" is not valid" + errMsgSuffix);
        return false;
    }
    logMsg(LOG_INFO, name + " depressurizationColor=" + rgbStr);

    if (!patternConfig["fillStepSize"].is_number()) {
        logMsg(LOG_ERR, "fillStepSize not specified" + errMsgSuffix);
        return false;
    }
    fillStepSize = patternConfig["fillStepSize"].int_value();
    logMsg(LOG_INFO, name + " fillStepSize=" + to_string(fillStepSize));

    if (!patternConfig["fillStepIntervalMs"].is_number()) {
        logMsg(LOG_ERR, "fillStepIntervalMs not specified" + errMsgSuffix);
        return false;
    }
    fillStepIntervalMs = patternConfig["fillStepIntervalMs"].int_value();
    logMsg(LOG_INFO, name + " fillStepIntervalMs=" + to_string(fillStepIntervalMs));

    std::vector<Pattern::ChannelConfiguration> channelConfigs = getChannelConfigurations(config, widgets);
    if (channelConfigs.empty()) {
        logMsg(LOG_WARNING, "No valid widget channels are configured for " + name + ".");
        return false;
    }

    for (auto&& channelConfig : channelConfigs) {

        if (channelConfig.inputName == "pressure") {
            pressureChannel = channelConfig.widgetChannel;
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

    // If we're in one of the bursting states and it isn't time
    // to do the next step, just return that we're active.
    if (state != PatternState::pressurizing && state != PatternState::depressurizing
        && (int) (nowMs - nextStepMs) < 0)
    {
        return isActive;
    }

    int curMeasmt;

    switch (state) {

        case PatternState::pressurizing:

            if (!pressureChannel->getHasNewPositionMeasurement()) {
                return isActive;
            }
  
            curMeasmt = pressureChannel->getPosition();
            if (curMeasmt <= lowPressureCutoff) {
                isActive = false;
                return false;
            }

            isActive = true;

            clearAllPixels();

            // Start bursting if we're past the maximum pressure.
            if (curMeasmt > burstThreshold) {
                fillPosition = pixelsPerString;
                nextStepMs = nowMs;         // immediately
                state = PatternState::fillRed;
            }
            else {
                // Fill the cone from the bottom up to represent the current pressure.
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
                state = PatternState::endBursting;
            }
            break;

        case PatternState::endBursting:
            priority = fillingPriority;
            clearAllPixels();
            state = PatternState::depressurizing;
            break;

        case PatternState::depressurizing:
            // If the widget has gone inactive, we'll assume that
            // the pressure is below the low pressure cutoff.
            if (!pressureChannel->getIsActive()) {
                state = PatternState::pressurizing;
            }
            else {
                // We'll wait for the pressure to drop to or below the
                // low pressure cutoff before allowing pressurization again.
                if (pressureChannel->getHasNewPositionMeasurement()) {
                    curMeasmt = pressureChannel->getPosition();
                    if (curMeasmt <= lowPressureCutoff) {
                        isActive = false;
                        state = PatternState::pressurizing;
                    }
                    else {
                        // Fill the cone from the bottom up to represent the current pressure.
                        clearAllPixels();
                        int fillLevel = curMeasmt >= burstThreshold
                                      ? pixelsPerString
                                      : pixelsPerString * (curMeasmt - lowPressureCutoff) / (burstThreshold - lowPressureCutoff);
                        for (auto&& pixels:pixelArray) {
                            for (unsigned int i = pixelsPerString - fillLevel; i < pixelsPerString; i++) {
                                pixels[i] = depressurizationColor;
                            }
                        }
                    }
                }
            }
            break;
    }

    return isActive;
}
