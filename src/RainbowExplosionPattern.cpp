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

#include <chrono>
#include <iostream>
#include <time.h>

#include "ConfigReader.h"
#include "Pattern.h"
#include "RainbowExplosionPattern.h"
#include "Widget.h"
#include "WidgetChannel.h"


using namespace std;


RainbowExplosionPattern::RainbowExplosionPattern()
    : Pattern("rainbowExplosion")
{
}


bool RainbowExplosionPattern::initPattern(ConfigReader& config, std::map<WidgetId, Widget*>& widgets, int priority)
{
    numStrings = config.getNumberOfStrings();
    pixelsPerString = config.getNumberOfPixelsPerString();
    this->priority = priority;
    opacity = 100;

    pixelArray.resize(numStrings, std::vector<opc_pixel_t>(pixelsPerString));

    state = PatternState::fizzle;
    accumulator = 0;

    auto patternConfig = config.getPatternConfigJsonObject(name);

    if (!patternConfig["activationThreshold"].is_number()) {
        cerr << "activationThreshold not specified in " << name << " pattern configuration." << endl;
        return false;
    }
    activationThreshold = patternConfig["activationThreshold"].int_value();
    cout << name << " activationThreshold=" << activationThreshold << endl;

    if (!patternConfig["explosionThreshold"].is_number()) {
        cerr << "explosionThreshold not specified in " << name << " pattern configuration." << endl;
        return false;
    }
    explosionThreshold = patternConfig["explosionThreshold"].int_value();
    cout << name << " explosionThreshold=" << explosionThreshold << endl;

    if (!patternConfig["accumulatorResetUpperLimit"].is_number()) {
        cerr << "accumulatorResetUpperLimit not specified in " << name << " pattern configuration." << endl;
        return false;
    }
    accumulatorResetUpperLimit = patternConfig["accumulatorResetUpperLimit"].int_value();
    cout << name << " accumulatorResetUpperLimit=" << accumulatorResetUpperLimit << endl;

    if (!patternConfig["minFizzleFill"].is_number()) {
        cerr << "minFizzleFill not specified in " << name << " pattern configuration." << endl;
        return false;
    }
    minFizzleFill = patternConfig["minFizzleFill"].int_value();
    cout << name << " minFizzleFill=" << minFizzleFill << endl;

    if (!patternConfig["maxFizzleFill"].is_number()) {
        cerr << "maxFizzleFill not specified in " << name << " pattern configuration." << endl;
        return false;
    }
    maxFizzleFill = patternConfig["maxFizzleFill"].int_value();
    cout << name << " maxFizzleFill=" << maxFizzleFill << endl;

    if (!patternConfig["fillStepSize"].is_number()) {
        cerr << "fillStepSize not specified in " << name << " pattern configuration." << endl;
        return false;
    }
    fillStepSize = patternConfig["fillStepSize"].int_value();
    cout << name << " fillStepSize=" << fillStepSize << endl;

    if (!patternConfig["fillStepIntervalMs"].is_number()) {
        cerr << "fillStepIntervalMs not specified in " << name << " pattern configuration." << endl;
        return false;
    }
    fillStepIntervalMs = patternConfig["fillStepIntervalMs"].int_value();
    cout << name << " fillStepIntervalMs=" << fillStepIntervalMs << endl;

    std::vector<Pattern::ChannelConfiguration> channelConfigs = getChannelConfigurations(config, widgets);
    if (channelConfigs.empty()) {
        cerr << "No valid widget channels are configured for " << name << "." << endl;
        return false;
    }

    for (auto&& channelConfig : channelConfigs) {

        if (channelConfig.inputName == "intensity") {
            intensityChannel = channelConfig.widgetChannel;
        }
        else {
            cerr << "Warning:  inputName '" << channelConfig.inputName
                << "' in input configuration for " << name << " is not recognized." << endl;
            continue;
        }
        cout << name << " using " << channelConfig.widgetChannel->getName() << " for " << channelConfig.inputName << endl;

        if (channelConfig.measurement != "position") {
            cerr << "Warning:  " << name << " supports only position measurements, but the input configuration for "
                << channelConfig.inputName << " doesn't specify position." << endl;
        }
    }

    return true;
}


void RainbowExplosionPattern::clearAllPixels()
{
    for (auto&& pixels:pixelArray) {
        for (auto&& pixel:pixels) {
            pixel.r = 0;
            pixel.g = 0;
            pixel.b = 0;
        }
    }
}


bool RainbowExplosionPattern::update()
{
    // Don't do anything if no input channel was assigned.
    if (intensityChannel == nullptr) {
        return false;
    }

    std::chrono::milliseconds epochMs =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
    unsigned int nowMs = epochMs.count();

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
                accumulator = rand() % accumulatorResetUpperLimit;
                fillPosition = pixelsPerString;
                nextStepMs = nowMs;         // immediately
                state = PatternState::fillRed;
            } else {
                accumulator++;
                // Fill the cone with red from the bottom up to a random depth.
                for (auto&& pixels:pixelArray) {
                    int fillLevel = (rand() + minFizzleFill) % maxFizzleFill;
                    for (int i = pixelsPerString - fillLevel; i < pixelsPerString; i++) {
                        pixels[i].r = 127;
                        pixels[i].g = 0;
                        pixels[i].b = 0;
                    }
                }
                fizzleMeasurementTimeoutMs = nowMs + fizzleMeasurementTimeoutPeriodMs;
            }
            break;

        case PatternState::fillRed:
            //cout << "fillRed" << endl;
            fillPosition = max(fillPosition - fillStepSize, 0);
            for (int i = fillPosition; i < pixelsPerString; i++) {
                for (auto&& pixels:pixelArray) {
                    pixels[i].r = 255;
                    pixels[i].g = 0;
                    pixels[i].b = 0;
                }
            }
            nextStepMs = nowMs + fillStepIntervalMs;
            if (fillPosition <= 0) {
                fillPosition = pixelsPerString;
                state = PatternState::fillOrange;
            }
            break;

        case PatternState::fillOrange:
            //cout << "fillOrange" << endl;
            fillPosition = max(fillPosition - fillStepSize, 0);
            for (int i = fillPosition; i < pixelsPerString; i++) {
                for (auto&& pixels:pixelArray) {
                    pixels[i].r = 255;
                    pixels[i].g = 127;
                    pixels[i].b = 0;
                }
            }
            nextStepMs = nowMs + fillStepIntervalMs;
            if (fillPosition <= 0) {
                fillPosition = pixelsPerString;
                state = PatternState::fillYellow;
            }
            break;

        case PatternState::fillYellow:
            //cout << "fillYellow" << endl;
            fillPosition = max(fillPosition - fillStepSize, 0);
            for (int i = fillPosition; i < pixelsPerString; i++) {
                for (auto&& pixels:pixelArray) {
                    pixels[i].r = 255;
                    pixels[i].g = 255;
                    pixels[i].b = 0;
                }
            }
            nextStepMs = nowMs + fillStepIntervalMs;
            if (fillPosition <= 0) {
                fillPosition = pixelsPerString;
                state = PatternState::fillGreen;
            }
            break;

        case PatternState::fillGreen:
            //cout << "fillGreen" << endl;
            fillPosition = max(fillPosition - fillStepSize, 0);
            for (int i = fillPosition; i < pixelsPerString; i++) {
                for (auto&& pixels:pixelArray) {
                    pixels[i].r = 0;
                    pixels[i].g = 255;
                    pixels[i].b = 0;
                }
            }
            nextStepMs = nowMs + fillStepIntervalMs;
            if (fillPosition <= 0) {
                fillPosition = pixelsPerString;
                state = PatternState::fillBlue;
            }
            break;

        case PatternState::fillBlue:
            //cout << "fillBlue" << endl;
            fillPosition = max(fillPosition - fillStepSize, 0);
            for (int i = fillPosition; i < pixelsPerString; i++) {
                for (auto&& pixels:pixelArray) {
                    pixels[i].r = 0;
                    pixels[i].g = 0;
                    pixels[i].b = 255;
                }
            }
            nextStepMs = nowMs + fillStepIntervalMs;
            if (fillPosition <= 0) {
                fillPosition = pixelsPerString;
                state = PatternState::fillIndigo;
            }
            break;

        case PatternState::fillIndigo:
            //cout << "fillIndigo" << endl;
            fillPosition = max(fillPosition - fillStepSize, 0);
            for (int i = fillPosition; i < pixelsPerString; i++) {
                for (auto&& pixels:pixelArray) {
                    pixels[i].r = 75;
                    pixels[i].g = 0;
                    pixels[i].b = 130;
                }
            }
            nextStepMs = nowMs + fillStepIntervalMs;
            if (fillPosition <= 0) {
                fillPosition = pixelsPerString;
                state = PatternState::fillViolet;
            }
            break;

        case PatternState::fillViolet:
            //cout << "fillViolet" << endl;
            fillPosition = max(fillPosition - fillStepSize, 0);
            for (int i = fillPosition; i < pixelsPerString; i++) {
                for (auto&& pixels:pixelArray) {
                    pixels[i].r = 148;
                    pixels[i].g = 0;
                    pixels[i].b = 211;
                }
            }
            nextStepMs = nowMs + fillStepIntervalMs;
            if (fillPosition <= 0) {
                state = PatternState::endExplosion;
            }
            break;

        case PatternState::endExplosion:
            //cout << "endExplosion" << endl;
            for (auto&& pixels:pixelArray) {
                for (auto&& pixel:pixels) {
                    pixel.r = 0;
                    pixel.g = 0;
                    pixel.b = 0;
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
