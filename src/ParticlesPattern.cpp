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
#include "Log.h"
#include "ParticlesPattern.h"
#include "Pattern.h"
#include "Widget.h"
#include "WidgetChannel.h"

using namespace std;


extern Log logger;


ParticlesPattern::ParticlesPattern(const std::string& name)
    : Pattern(name)
{
}


bool ParticlesPattern::initPattern(std::map<WidgetId, Widget*>& widgets)
{
    // ----- get input channels -----

    std::vector<Pattern::ChannelConfiguration> channelConfigs = getChannelConfigurations(widgets);
    if (channelConfigs.empty()) {
        logger.logMsg(LOG_ERR, "No valid widget channels are configured for " + name + ".");
        return false;
    }

    for (auto&& channelConfig : channelConfigs) {

        if (channelConfig.inputName == "emitRate") {
            emitRateChannel = channelConfig.widgetChannel;
            if (channelConfig.measurement == "velocity") {
                emitRateUsePositionMeasmt = false;
            }
            else if (channelConfig.measurement == "position") {
                emitRateUsePositionMeasmt = true;
            }
            else {
                logger.logMsg(LOG_ERR, channelConfig.inputName + " must specify position or velocity for " + name + ".");
                return false;
            }
            logger.logMsg(LOG_INFO, name + " using " + channelConfig.widgetChannel->getName()
                             + (emitRateUsePositionMeasmt ? " position measurement for " : " velocity measurement for ")
                             + channelConfig.inputName);
        }
        else if (channelConfig.inputName == "emitColor") {
            emitColorChannel = channelConfig.widgetChannel;
            if (channelConfig.measurement == "velocity") {
                emitColorUsePositionMeasmt = false;
            }
            else if (channelConfig.measurement == "position") {
                emitColorUsePositionMeasmt = true;
            }
            else {
                logger.logMsg(LOG_ERR, channelConfig.inputName + " must specify position or velocity for " + name + ".");
                return false;
            }
            logger.logMsg(LOG_INFO, name + " using " + channelConfig.widgetChannel->getName()
                             + (emitColorUsePositionMeasmt ? " position measurement for " : " velocity measurement for ")
                             + channelConfig.inputName);
        }
        else {
            logger.logMsg(LOG_WARNING, "inputName '" + channelConfig.inputName
                                + "' in input configuration for " + name + " is not recognized.");
            continue;
        }

    }

    // ----- get pattern configuration -----

    string errMsgSuffix = " in " + name + " pattern configuration.";

    if (!ConfigReader::getBoolValue(patternConfigObject, "emitIntervalUseMeasmtAbsValue", emitIntervalUseMeasmtAbsValue, errMsgSuffix)) {
        return false;
    }
    logger.logMsg(LOG_INFO, name + " emitIntervalUseMeasmtAbsValue=" + (emitIntervalUseMeasmtAbsValue ? "true" : "false"));

    if (!ConfigReader::getIntValue(patternConfigObject, "emitIntervalMeasmtLow", emitIntervalMeasmtLow, errMsgSuffix)) {
        return false;
    }
    logger.logMsg(LOG_INFO, name + " emitIntervalMeasmtLow=" + to_string(emitIntervalMeasmtLow));

    if (!ConfigReader::getIntValue(patternConfigObject, "emitIntervalMeasmtHigh", emitIntervalMeasmtHigh, errMsgSuffix)) {
        return false;
    }
    logger.logMsg(LOG_INFO, name + " emitIntervalMeasmtHigh=" + to_string(emitIntervalMeasmtHigh));

    if (!ConfigReader::getIntValue(patternConfigObject, "emitIntervalLowMs", emitIntervalLowMs, errMsgSuffix)) {
        return false;
    }
    logger.logMsg(LOG_INFO, name + " emitIntervalLowMs=" + to_string(emitIntervalLowMs));

    if (!ConfigReader::getIntValue(patternConfigObject, "emitIntervalHighMs", emitIntervalHighMs, errMsgSuffix, 1)) {
        return false;
    }
    logger.logMsg(LOG_INFO, name + " emitIntervalHighMs=" + to_string(emitIntervalHighMs));

    if (!ConfigReader::getIntValue(patternConfigObject, "emitBatchSize", emitBatchSize, errMsgSuffix, 1)) {
        return false;
    }
    logger.logMsg(LOG_INFO, name + " emitBatchSize=" + to_string(emitBatchSize));

    if (!ConfigReader::getBoolValue(patternConfigObject, "emitDirectionIsUp", emitDirectionIsUp, errMsgSuffix)) {
        return false;
    }
    logger.logMsg(LOG_INFO, name + " emitDirectionIsUp=" + to_string(emitDirectionIsUp));

    if (!ConfigReader::getUnsignedIntValue(patternConfigObject, "particleMoveIntervalMs", particleMoveIntervalMs, errMsgSuffix, 1)) {
        return false;
    }
    logger.logMsg(LOG_INFO, name + " particleMoveIntervalMs=" + to_string(particleMoveIntervalMs));

    // --- emit color stuff ---

    string hsvStr;

    if (!ConfigReader::getHsvPixelValue(patternConfigObject, "emitColorDefault", hsvStr, emitColorDefault, errMsgSuffix)) {
        return false;
    }
    logger.logMsg(LOG_INFO, name + " emitColorDefault=" + hsvStr);

    if (emitColorChannel != nullptr) {

        if (!ConfigReader::getDoubleValue(patternConfigObject, "emitColorMeasmtLow", emitColorMeasmtLow, errMsgSuffix)) {
            return false;
        }
        logger.logMsg(LOG_INFO, name + " emitColorMeasmtLow=" + to_string(emitColorMeasmtLow));

        if (!ConfigReader::getDoubleValue(patternConfigObject, "emitColorMeasmtHigh", emitColorMeasmtHigh, errMsgSuffix)) {
            return false;
        }
        logger.logMsg(LOG_INFO, name + " emitColorMeasmtHigh=" + to_string(emitColorMeasmtHigh));

        emitColorMeasmtRange = emitColorMeasmtHigh - emitColorMeasmtLow;

        if (!ConfigReader::getHsvPixelValue(patternConfigObject, "emitColorLow", hsvStr, emitColorLow, errMsgSuffix)) {
            return false;
        }
        logger.logMsg(LOG_INFO, name + " emitColorLow=" + hsvStr);

        string hsvStr;
        if (!ConfigReader::getHsvPixelValue(patternConfigObject, "emitColorHigh", hsvStr, emitColorHigh, errMsgSuffix)) {
            return false;
        }
        logger.logMsg(LOG_INFO, name + " emitColorHigh=" + hsvStr);

        // Setting the high and low colors to the same value
        // means that we should use the entire color wheel.
        if (emitColorHigh == emitColorLow) {
            emitColorLow.h = 0;
            emitColorHigh.h = 255;
        }
        logger.logMsg(LOG_INFO, name + " emitColorLow.h=" + to_string(emitColorLow.h));
        logger.logMsg(LOG_INFO, name + " emitColorHigh.h=" + to_string(emitColorHigh.h));

        emitColorHueRange = emitColorHigh.h - emitColorLow.h;

        if (!ConfigReader::getDoubleValue(patternConfigObject, "emitColorMeasmtMultiplier", emitColorMeasmtMultiplier, errMsgSuffix)) {
            return false;
        }
        logger.logMsg(LOG_INFO, name + " emitColorMeasmtMultiplier=" + to_string(emitColorMeasmtMultiplier));

        if (!ConfigReader::getBoolValue(patternConfigObject, "emitColorIntegrateMeasmt", emitColorIntegrateMeasmt, errMsgSuffix)) {
            return false;
        }
        logger.logMsg(LOG_INFO, name + " emitColorIntegrateMeasmt=" + to_string(emitColorIntegrateMeasmt));
        if (emitColorIntegrateMeasmt) {
            logger.logMsg(LOG_ERR, "Integration of emit color measurments it not yet supported for " + name + ".");
            return false;
        }
    }

    // ----- initialize object data -----

    numRotationsNeededToClearParticles = 0;
    nextMoveParticlesMs = 0;
    nextEmitParticlesMs = 0;
    hsv2rgb(emitColorDefault, rgbEmitColor);

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
    // Don't do anything if no emit rate input channel was assigned.
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
        //logger.logMsg(LOG_DEBUG, "time to move particles");
        if (moveParticles()) {
            isActive = true;
            nextMoveParticlesMs = nowMs + particleMoveIntervalMs;
        }
        else {
            isActive = false;
            nextMoveParticlesMs = 0;
        }
    }

    // Update the emit color if we're directly using a scaled measurement
    // (i.e., not integrating the rate of change of something).
    if (!emitColorIntegrateMeasmt && emitColorChannel != nullptr && emitColorChannel->getIsActive()
        && ((emitColorUsePositionMeasmt && emitRateChannel->getHasNewPositionMeasurement())
            || (!emitColorUsePositionMeasmt && emitRateChannel->getHasNewVelocityMeasurement())))
    {
        //logger.logMsg(LOG_DEBUG, "emitColorChannel has a new measurement");
        double emitColorMeasmt = emitColorUsePositionMeasmt ? emitColorChannel->getPosition() : emitColorChannel->getVelocity();
        emitColorMeasmt *= emitColorMeasmtMultiplier;
        emitColorMeasmt = std::min(emitColorMeasmt, emitColorMeasmtHigh);
        emitColorMeasmt = std::max(emitColorMeasmt, emitColorMeasmtLow);

        double emitColorHue = emitColorHueRange * (emitColorMeasmt - emitColorMeasmtLow) / emitColorMeasmtRange + emitColorLow.h;
        //logger.logMsg(LOG_DEBUG,
        //    "emitColorLow.h=" + to_string(emitColorLow.h)
        //    + ", emitColorHigh.h=" + to_string(emitColorHigh.h)
        //    + ", emitColorMeasmt=" + to_string(emitColorMeasmt)
        //    + ", emitColorHue=" + to_string(emitColorHue));

        CHSV hsvEmitColor((uint8_t) emitColorHue, 255, 255);
        hsv2rgb(hsvEmitColor, rgbEmitColor);
        //string hsvEmitColorStr;
        //hsvPixelToString(hsvEmitColor, hsvEmitColorStr);
        //string rgbEmitColorStr;
        //rgbPixelToString(rgbEmitColor, rgbEmitColorStr);
        //logger.logMsg(LOG_DEBUG, "hsvEmitColor=" + hsvEmitColorStr + "  rgbEmitColor=" + rgbEmitColorStr);
    }

    // Don't emit any particles if the widget has gone inactive.
    if (!emitRateChannel->getIsActive()) {
        //logger.logMsg(LOG_DEBUG, "emitRateChannel is inactive");
        nextEmitParticlesMs = 0;
        return wasActive;
    }

    // If there is a new measurement, update the emit rate.
    if ((emitRateUsePositionMeasmt && emitRateChannel->getHasNewPositionMeasurement())
        || (!emitRateUsePositionMeasmt && emitRateChannel->getHasNewVelocityMeasurement()))
    {
        //logger.logMsg(LOG_DEBUG, "emitRateChannel has a new measurement");

        int emitIntervalMeasmt = emitRateUsePositionMeasmt ? emitRateChannel->getPosition() : emitRateChannel->getVelocity();
        if (emitIntervalUseMeasmtAbsValue) {
            emitIntervalMeasmt = abs(emitIntervalMeasmt);
        }

        // Don't emit anything if the measurement is below the lower limit.
        if (emitIntervalMeasmt < emitIntervalMeasmtLow) {
            //logger.logMsg(LOG_DEBUG, "emitIntervalMeasmt is below the lower limit");
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
            //logger.logMsg(LOG_DEBUG,
            //    "emitIntervalMeasmt=" + to_string(emitIntervalMeasmt)
            //    + ", particleEmitIntervalMs=" + to_string(particleEmitIntervalMs)
            //    + ", nextEmitParticlesMs=" + to_string(nextEmitParticlesMs));
        }
    }

    //logger.logMsg(LOG_DEBUG,
    //    "nowMs=" + to_string(nowMs)
    //    + ", nextEmitParticlesMs=" + to_string(nextEmitParticlesMs)
    //    + ", particleEmitIntervalMs=" + to_string(particleEmitIntervalMs));

    // Emit some particles if it is time to do so.
    if (nextEmitParticlesMs > 0 && (int) (nowMs - nextEmitParticlesMs) >= 0) {
        nextEmitParticlesMs = nowMs + particleEmitIntervalMs;

        //logger.logMsg(LOG_DEBUG, "time to emit particles");

        // Emit particles.
        for (int i = 0; i < emitBatchSize; ++i) {
            int randStringNum = random16(numStrings);
            pixelArray[randStringNum][emitDirectionIsUp ? pixelsPerString - 1 : 0] = rgbEmitColor;
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

