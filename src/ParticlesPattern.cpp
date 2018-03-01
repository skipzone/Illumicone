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
#include "illumiconePixelUtility.h"
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


    // ----- get input channels -----

    std::vector<Pattern::ChannelConfiguration> channelConfigs = getChannelConfigurations(config, widgets);
    if (channelConfigs.empty()) {
        logMsg(LOG_ERR, "No valid widget channels are configured for " + name + ".");
        return false;
    }

    for (auto&& channelConfig : channelConfigs) {

        if (channelConfig.inputName == "emitRate") {
            emitRateChannel = channelConfig.widgetChannel;
        }
        else if (channelConfig.inputName == "emitColor") {
            emitColorChannel = channelConfig.widgetChannel;
        }
        else {
            logMsg(LOG_WARNING, "inputName '" + channelConfig.inputName
                + "' in input configuration for " + name + " is not recognized.");
            continue;
        }

        if (channelConfig.measurement == "velocity") {
            usePositionMeasurement = false;
        }
        else if (channelConfig.measurement == "position") {
            usePositionMeasurement = true;
        }
        else {
            logMsg(LOG_ERR, channelConfig.inputName + " must specify position or velocity for " + name + ".");
            return false;
        }

        logMsg(LOG_INFO, name + " using " + channelConfig.widgetChannel->getName()
                         + (usePositionMeasurement ? " position measurement for " : " velocity measurement for ")
                         + channelConfig.inputName);
    }

    // ----- get pattern configuration -----

    string errMsgSuffix = " in " + name + " pattern configuration.";

    auto patternConfig = config.getPatternConfigJsonObject(name);

    if (!ConfigReader::getIntValue(patternConfig, "emitIntervalMeasmtLow", emitIntervalMeasmtLow, errMsgSuffix)) {
        return false;
    }
    logMsg(LOG_INFO, name + " emitIntervalMeasmtLow=" + to_string(emitIntervalMeasmtLow));

    if (!ConfigReader::getIntValue(patternConfig, "emitIntervalMeasmtHigh", emitIntervalMeasmtHigh, errMsgSuffix)) {
        return false;
    }
    logMsg(LOG_INFO, name + " emitIntervalMeasmtHigh=" + to_string(emitIntervalMeasmtHigh));

    if (!ConfigReader::getIntValue(patternConfig, "emitIntervalLowMs", emitIntervalLowMs, errMsgSuffix)) {
        return false;
    }
    logMsg(LOG_INFO, name + " emitIntervalLowMs=" + to_string(emitIntervalLowMs));

    if (!ConfigReader::getIntValue(patternConfig, "emitIntervalHighMs", emitIntervalHighMs, errMsgSuffix, 1)) {
        return false;
    }
    logMsg(LOG_INFO, name + " emitIntervalHighMs=" + to_string(emitIntervalHighMs));

    if (!ConfigReader::getIntValue(patternConfig, "emitBatchSize", emitBatchSize, errMsgSuffix, 1)) {
        return false;
    }
    logMsg(LOG_INFO, name + " emitBatchSize=" + to_string(emitBatchSize));

    if (!ConfigReader::getBoolValue(patternConfig, "emitDirectionIsUp", emitDirectionIsUp, errMsgSuffix)) {
        return false;
    }
    logMsg(LOG_INFO, name + " emitDirectionIsUp=" + to_string(emitDirectionIsUp));

    if (!ConfigReader::getUnsignedIntValue(patternConfig, "particleMoveIntervalMs", particleMoveIntervalMs, errMsgSuffix, 1)) {
        return false;
    }
    logMsg(LOG_INFO, name + " particleMoveIntervalMs=" + to_string(particleMoveIntervalMs));

    // --- emit color stuff ---

    string hsvStr;

    if (!ConfigReader::getHsvPixelValue(patternConfig, "emitColorDefault", hsvStr, emitColorDefault, errMsgSuffix)) {
        return false;
    }
    logMsg(LOG_INFO, name + " emitColorDefault=" + hsvStr);

    if (emitColorChannel != nullptr) {
/*
*/

        if (!ConfigReader::getDoubleValue(patternConfig, "emitColorMeasmtLow", emitColorMeasmtLow, errMsgSuffix)) {
            return false;
        }
        logMsg(LOG_INFO, name + " emitColorMeasmtLow=" + to_string(emitColorMeasmtLow));

        if (!ConfigReader::getDoubleValue(patternConfig, "emitColorMeasmtHigh", emitColorMeasmtHigh, errMsgSuffix)) {
            return false;
        }
        logMsg(LOG_INFO, name + " emitColorMeasmtHigh=" + to_string(emitColorMeasmtHigh));

        if (!ConfigReader::getHsvPixelValue(patternConfig, "emitColorLow", hsvStr, emitColorLow, errMsgSuffix)) {
            return false;
        }
        logMsg(LOG_INFO, name + " emitColorLow=" + hsvStr);

        string hsvStr;
        if (!ConfigReader::getHsvPixelValue(patternConfig, "emitColorHigh", hsvStr, emitColorHigh, errMsgSuffix)) {
            return false;
        }
        logMsg(LOG_INFO, name + " emitColorHigh=" + hsvStr);

        if (!ConfigReader::getDoubleValue(patternConfig, "emitColorMeasmtMultiplier", emitColorMeasmtMultiplier, errMsgSuffix)) {
            return false;
        }
        logMsg(LOG_INFO, name + " emitColorMeasmtMultiplier=" + to_string(emitColorMeasmtMultiplier));

        if (!ConfigReader::getBoolValue(patternConfig, "emitColorIntegrateMeasmt", emitColorIntegrateMeasmt, errMsgSuffix)) {
            return false;
        }
        logMsg(LOG_INFO, name + " emitColorIntegrateMeasmt=" + to_string(emitColorIntegrateMeasmt));
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
    // TODO 8/29/2017 ross:  This wasActive thing is unnecessary.  When the pattern goes inactive, its frame will be ignored,
    //                       essentially turning off the display of it.
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
    if ((usePositionMeasurement && emitRateChannel->getHasNewPositionMeasurement())
        || (!usePositionMeasurement && emitRateChannel->getHasNewVelocityMeasurement()))
    {
        //logMsg(LOG_DEBUG, "emitRateChannel has a new measurement");

        int emitIntervalMeasmt = usePositionMeasurement ? emitRateChannel->getPosition() : abs(emitRateChannel->getVelocity());

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
        CRGB rgbEmitColorDefault;
        hsv2rgb(emitColorDefault, rgbEmitColorDefault);
        for (int i = 0; i < emitBatchSize; ++i) {
            int randStringNum = random16(numStrings);
            pixelArray[randStringNum][emitDirectionIsUp ? pixelsPerString - 1 : 0] = rgbEmitColorDefault;
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

