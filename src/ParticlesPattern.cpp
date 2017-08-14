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
#include "log.h"
#include "ParticlesPattern.h"
#include "Pattern.h"
#include "Widget.h"
#include "WidgetChannel.h"


using namespace std;


ParticlesPattern::ParticlesPattern(const std::string& name)
    : Pattern(name)
{
}


bool ParticlesPattern::initPattern(ConfigReader& config, std::map<WidgetId, Widget*>& widgets)
{
    numRotationsNeededToClearParticles = 0;
    nextMoveParticlesMs = 0;
    nextEmitParticlesMs = 0;

    // ----- get pattern configuration -----

    auto patternConfig = config.getPatternConfigJsonObject(name);

    if (patternConfig["emitColorRedValue"].is_number()) {
        emitColor.r = patternConfig["emitColorRedValue"].int_value();
    }
    if (patternConfig["emitColorGreenValue"].is_number()) {
        emitColor.g = patternConfig["emitColorGreenValue"].int_value();
    }
    if (patternConfig["emitColorBlueValue"].is_number()) {
        emitColor.b = patternConfig["emitColorBlueValue"].int_value();
    }
    if (emitColor == CRGB(CRGB::Black)) {
        logMsg(LOG_ERR, "No emit color values are specified in " + name + " pattern configuration.");
        return false;
    }
    logMsg(LOG_INFO, name
            + " emitColor r=" + to_string(emitColor.r)
            + ", g=" + to_string(emitColor.g)
            + ", b=" + to_string(emitColor.b) );

    if (!patternConfig["emitIntervalMeasmtLow"].is_number()) {
        logMsg(LOG_ERR, "emitIntervalMeasmtLow not specified in " + name + " pattern configuration.");
        return false;
    }
    emitIntervalMeasmtLow = patternConfig["emitIntervalMeasmtLow"].int_value();
    logMsg(LOG_INFO, name + " emitIntervalMeasmtLow=" + to_string(emitIntervalMeasmtLow));

    if (!patternConfig["emitIntervalMeasmtHigh"].is_number()) {
        logMsg(LOG_ERR, "emitIntervalMeasmtHigh not specified in " + name + " pattern configuration.");
        return false;
    }
    emitIntervalMeasmtHigh = patternConfig["emitIntervalMeasmtHigh"].int_value();
    logMsg(LOG_INFO, name + " emitIntervalMeasmtHigh=" + to_string(emitIntervalMeasmtHigh));

    if (!patternConfig["emitIntervalLowMs"].is_number()) {
        logMsg(LOG_ERR, "emitIntervalLowMs not specified in " + name + " pattern configuration.");
        return false;
    }
    emitIntervalLowMs = patternConfig["emitIntervalLowMs"].int_value();
    logMsg(LOG_INFO, name + " emitIntervalLowMs=" + to_string(emitIntervalLowMs));

    if (!patternConfig["emitIntervalHighMs"].is_number()) {
        logMsg(LOG_ERR, "emitIntervalHighMs not specified in " + name + " pattern configuration.");
        return false;
    }
    emitIntervalHighMs = patternConfig["emitIntervalHighMs"].int_value();
    logMsg(LOG_INFO, name + " emitIntervalHighMs=" + to_string(emitIntervalHighMs));

    if (!patternConfig["emitBatchSize"].is_number()) {
        logMsg(LOG_ERR, "emitBatchSize not specified in " + name + " pattern configuration.");
        return false;
    }
    emitBatchSize = patternConfig["emitBatchSize"].int_value();
    logMsg(LOG_INFO, name + " emitBatchSize=" + to_string(emitBatchSize));

    if (!patternConfig["emitDirectionIsUp"].is_bool()) {
        logMsg(LOG_ERR, "emitDirectionIsUp not specified in " + name + " pattern configuration.");
        return false;
    }
    emitDirectionIsUp = patternConfig["emitDirectionIsUp"].bool_value();
    logMsg(LOG_INFO, name + " emitDirectionIsUp=" + to_string(emitDirectionIsUp));

    if (!patternConfig["particleMoveIntervalMs"].is_number()) {
        logMsg(LOG_ERR, "particleMoveIntervalMs not specified in " + name + " pattern configuration.");
        return false;
    }
    particleMoveIntervalMs = patternConfig["particleMoveIntervalMs"].int_value();
    logMsg(LOG_INFO, name + " particleMoveIntervalMs=" + to_string(particleMoveIntervalMs));

    // ----- get input channels -----

    // 0:  min sound sample
    // 1:  max sound sample
    // 2:  sound amplitude
    // 3:  yaw
    // 4:  pitch
    // 5:  roll
    // 6:  0


    std::vector<Pattern::ChannelConfiguration> channelConfigs = getChannelConfigurations(config, widgets);
    if (channelConfigs.empty()) {
        logMsg(LOG_ERR, "No valid widget channels are configured for " + name + ".");
        return false;
    }

    for (auto&& channelConfig : channelConfigs) {

        if (channelConfig.inputName == "emitRate") {
            emitRateChannel = channelConfig.widgetChannel;
        }
        else {
            logMsg(LOG_WARNING, "Warning:  inputName '" + channelConfig.inputName
                + "' in input configuration for " + name + " is not recognized.");
            continue;
        }
        logMsg(LOG_INFO, name + " using " + channelConfig.widgetChannel->getName() + " for " + channelConfig.inputName);

        if (channelConfig.measurement != "position") {
            logMsg(LOG_WARNING, "Warning:  " + name + " supports only position measurements, but the input configuration for "
                + channelConfig.inputName + " doesn't specify position.");
        }
    }

    return true;
}


bool ParticlesPattern::moveParticles()
{
    if (numRotationsNeededToClearParticles > 0) {
        --numRotationsNeededToClearParticles;
        for (auto&& stringPixels : pixelArray) {
            unsigned int i;
            if (!emitDirectionIsUp) {
                for (i = stringPixels.size() - 1; i > 0; --i) {
                    stringPixels[i] = stringPixels[i - 1];
                }
            }
            else {
                for (i = 0; i < (unsigned int) (stringPixels.size() - 1); ++i) {
                    stringPixels[i] = stringPixels[i + 1];
                }
            }
            stringPixels[i] = CRGB::Black;
        }
    }

    return numRotationsNeededToClearParticles > 0 ? true : false;
}


bool ParticlesPattern::update()
{
    // Don't do anything if no input channel was assigned.
    if (emitRateChannel == nullptr) {
        return false;
    }

    // We'll return the previous activity state so that the final
    // frame will be displayed as this pattern goes inactive.
    bool wasActive = isActive;

    unsigned int nowMs = getNowMs();

    // Move the existing particles if it is time to do so.
    if (isActive && (int) (nowMs - nextMoveParticlesMs) >= 0) {
        //logMsg(LOG_DEBUG, "time to move particles");
        if (moveParticles()) {
            isActive = true;
            nextMoveParticlesMs = nowMs + particleMoveIntervalMs;
        }
        else {
            isActive = false;
            nextMoveParticlesMs = 0;
        }
    }

    // Don't emit any particles if the widget has gone inactive.
    if (!emitRateChannel->getIsActive()) {
        //logMsg(LOG_DEBUG, "emitRateChannel is inactive");
        nextEmitParticlesMs = 0;
        return wasActive;
    }

    // If there is a new measurement, update the emit rate.
    if (emitRateChannel->getHasNewPositionMeasurement()) {
        //logMsg(LOG_DEBUG, "emitRateChannel has a new measurement");

        int emitIntervalMeasmt = emitRateChannel->getPosition();

        // Don't emit anything if the measurement is below the lower limit.
        if (emitIntervalMeasmt < emitIntervalMeasmtLow) {
            //logMsg(LOG_DEBUG, "emitIntervalMeasmt is below the lower limit");
            nextEmitParticlesMs = 0;
        }
        else {
            // Map the emit interval measurement into the actual emit interval millisecond range.
            if (emitIntervalMeasmt > emitIntervalMeasmtHigh) {
                emitIntervalMeasmt = emitIntervalMeasmtHigh;
            }
            int emitIntervalMeasmtRange = emitIntervalMeasmtHigh - emitIntervalMeasmtLow;
            int emitIntervalRange = emitIntervalHighMs - emitIntervalLowMs;
            particleEmitIntervalMs =
                emitIntervalRange * (emitIntervalMeasmt - emitIntervalMeasmtLow) / emitIntervalMeasmtRange + emitIntervalLowMs;
            // If we're not currently emitting particles, start doing so immediately.
            if (nextEmitParticlesMs == 0) {
                nextEmitParticlesMs = nowMs;
            }
            //logMsg(LOG_DEBUG,
            //    "emitIntervalMeasmt=" + to_string(emitIntervalMeasmt)
            //    + ", particleEmitIntervalMs=" + to_string(particleEmitIntervalMs)
            //    + ", nextEmitParticlesMs=" + to_string(nextEmitParticlesMs));
        }
    }

    //logMsg(LOG_DEBUG,
    //    "nowMs=" + to_string(nowMs)
    //    + ", nextEmitParticlesMs=" + to_string(nextEmitParticlesMs)
    //    + ", particleEmitIntervalMs=" + to_string(particleEmitIntervalMs));

    // Emit some particles if it is time to do so.
    if (nextEmitParticlesMs > 0 && (int) (nowMs - nextEmitParticlesMs) >= 0) {
        nextEmitParticlesMs = nowMs + particleEmitIntervalMs;

        //logMsg(LOG_DEBUG, "time to emit particles");

        // Emit particles.
        for (int i = 0; i < emitBatchSize; ++i) {
            int randStringNum = rand() % numStrings;
            pixelArray[randStringNum][emitDirectionIsUp ? pixelsPerString - 1 : 0] = emitColor;
        }

        // Make sure the new particles eventually get moved out of the frame.
        numRotationsNeededToClearParticles = pixelsPerString;

        // Start particle movement if it isn't already in progress.
        if (nextMoveParticlesMs == 0) {
            nextMoveParticlesMs = nowMs + particleMoveIntervalMs;
        }

        isActive = true;
    }

    return wasActive;
}

