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

#include <stdlib.h>

#include "ConfigReader.h"
#include "illumiconeUtility.h"
#include "Log.h"
#include "Pattern.h"
#include "RainbowExplosionPattern.h"
#include "Widget.h"
#include "WidgetChannel.h"

using namespace std;


extern Log logger;


RainbowExplosionPattern::RainbowExplosionPattern(const std::string& name)
    : Pattern(name)
{
}


bool RainbowExplosionPattern::initPattern(ConfigReader& config, std::map<WidgetId, Widget*>& widgets)
{
    state = PatternState::fizzle;
    accumulator = 0;

    auto patternConfig = config.getPatternConfigJsonObject(name);

    if (!patternConfig["activationThreshold"].is_number()) {
        logger.logMsg(LOG_ERR, "activationThreshold not specified in " + name + " pattern configuration.");
        return false;
    }
    activationThreshold = patternConfig["activationThreshold"].int_value();
    logger.logMsg(LOG_INFO, name + " activationThreshold=" + to_string(activationThreshold));

    if (!patternConfig["explosionThreshold"].is_number()) {
        logger.logMsg(LOG_ERR, "explosionThreshold not specified in " + name + " pattern configuration.");
        return false;
    }
    explosionThreshold = patternConfig["explosionThreshold"].int_value();
    logger.logMsg(LOG_INFO, name + " explosionThreshold=" + to_string(explosionThreshold));

    if (!patternConfig["accumulatorResetUpperLimit"].is_number()) {
        logger.logMsg(LOG_ERR, "accumulatorResetUpperLimit not specified in " + name + " pattern configuration.");
        return false;
    }
    accumulatorResetUpperLimit = patternConfig["accumulatorResetUpperLimit"].int_value();
    logger.logMsg(LOG_INFO, name + " accumulatorResetUpperLimit=" + to_string(accumulatorResetUpperLimit));

    if (!patternConfig["minFizzleFill"].is_number()) {
        logger.logMsg(LOG_ERR, "minFizzleFill not specified in " + name + " pattern configuration.");
        return false;
    }
    minFizzleFill = patternConfig["minFizzleFill"].int_value();
    logger.logMsg(LOG_INFO, name + " minFizzleFill=" + to_string(minFizzleFill));

    if (!patternConfig["maxFizzleFill"].is_number()) {
        logger.logMsg(LOG_ERR, "maxFizzleFill not specified in " + name + " pattern configuration.");
        return false;
    }
    maxFizzleFill = patternConfig["maxFizzleFill"].int_value();
    logger.logMsg(LOG_INFO, name + " maxFizzleFill=" + to_string(maxFizzleFill));

    if (!patternConfig["fillStepSize"].is_number()) {
        logger.logMsg(LOG_ERR, "fillStepSize not specified in " + name + " pattern configuration.");
        return false;
    }
    fillStepSize = patternConfig["fillStepSize"].int_value();
    logger.logMsg(LOG_INFO, name + " fillStepSize=" + to_string(fillStepSize));

    if (!patternConfig["fillStepIntervalMs"].is_number()) {
        logger.logMsg(LOG_ERR, "fillStepIntervalMs not specified in " + name + " pattern configuration.");
        return false;
    }
    fillStepIntervalMs = patternConfig["fillStepIntervalMs"].int_value();
    logger.logMsg(LOG_INFO, name + " fillStepIntervalMs=" + to_string(fillStepIntervalMs));

    std::vector<Pattern::ChannelConfiguration> channelConfigs = getChannelConfigurations(config, widgets);
    if (channelConfigs.empty()) {
        logger.logMsg(LOG_WARNING, "No valid widget channels are configured for " + name + ".");
        return false;
    }

    for (auto&& channelConfig : channelConfigs) {

        if (channelConfig.inputName == "intensity") {
            intensityChannel = channelConfig.widgetChannel;
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

    return true;
}


void RainbowExplosionPattern::clearAllPixels()
{
    for (auto&& pixels:pixelArray) {
        for (auto&& pixel:pixels) {
            pixel = CRGB::Black;
        }
    }
}


bool RainbowExplosionPattern::update()
{
    // Don't do anything if no input channel was assigned.
    if (intensityChannel == nullptr) {
        return false;
    }

    unsigned int nowMs = getNowMs();

    // If we're in one of the explosion states and it isn't time
    // to do the next step, just return that we're active.
    if (state != PatternState::fizzle && (int) (nowMs - nextStepMs) < 0) {
        return isActive;
    }

    int curMeasmt;

    switch (state) {

        case PatternState::fizzle:

            // If we haven't received a new measurement, what we'll do depends
            // on whether or not we're displaying a fizzle.  If we are displaying
            // a fizzle and another measurement hasn't been received in a timely
            // fashion, we'll clear the fizzle display and go inactive.  Otherwise,
            // we'll leave the current pattern in place.
            if (!intensityChannel->getHasNewPositionMeasurement()) {
                if (isActive && (int) (nowMs - fizzleMeasurementTimeoutMs) >= 0) {
                    clearAllPixels();
                    isActive = false;
                    // We'll return true this time so that the fizzle
                    // we just displayed will get turned off.
                    return true;
                }
                return isActive;
            }

            clearAllPixels();

            // A pump "counts" only if it registers above the activation threshold.
            curMeasmt = intensityChannel->getPosition();
            if (curMeasmt <= activationThreshold) {
                if (isActive) {
                    isActive = false;
                    // We'll return true this time so that a fizzle we
                    // might have just displayed will get turned off.
                    return true;
                }
                return false;
            }
            isActive = true;

            // Each time we get a measurement above the activation threshold, we
            // increment the accumulator and turn a random portion of the bottom of
            // the cone red.  When the accumulator exceeds the explosion threshold,
            // we reset the accumulator to a random value (to get more of a random
            // explosion response) and do the rainbow explosion.
            if (accumulator > explosionThreshold) {
                accumulator = random16(accumulatorResetUpperLimit);
                fillPosition = pixelsPerString;
                nextStepMs = nowMs;         // immediately
                state = PatternState::fillRed;
            } else {
                accumulator++;
                // Fill the cone with red from the bottom up to a random depth.
                for (auto&& pixels:pixelArray) {
                    int fillLevel = (random16() + minFizzleFill) % maxFizzleFill;
                    for (unsigned int i = pixelsPerString - fillLevel; i < pixelsPerString; i++) {
                        pixels[i] = CRGB::Maroon;
                    }
                }
                fizzleMeasurementTimeoutMs = nowMs + fizzleMeasurementTimeoutPeriodMs;
            }
            break;

        case PatternState::fillRed:
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
                state = PatternState::endExplosion;
            }
            break;

        case PatternState::endExplosion:
            for (auto&& pixels:pixelArray) {
                for (auto&& pixel:pixels) {
                    pixel = CRGB::Black;
                }
            }
            state = PatternState::fizzle;
            // We'll return true this time so that the last explosion color
            // will get turned off.  But, the pattern is going inactive.
            isActive = false;
            break;
    }

    return true;
}
