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

#include <algorithm>
#include <climits>
#include <iostream>

#include <math.h>
#include <stdlib.h>

#include "ConfigReader.h"
#include "illumiconeUtility.h"
#include "illumiconePixelUtility.h"
#include "lib8tion.h"
#include "Log.h"
#include "Pattern.h"
#include "SparklePattern.h"
#include "Widget.h"
#include "WidgetChannel.h"

using namespace std;


extern Log logger;


SparklePattern::SparklePattern(const std::string& name)
    : Pattern(name)
{
};


bool SparklePattern::initPattern(std::map<WidgetId, Widget*>& widgets)
{
    decayStartMs = 0;
    inactiveStartMs = 0; 


    // ----- get pattern configuration -----

    string errMsgSuffix = " in " + name + " pattern configuration.";

    if (!ConfigReader::getIntValue(patternConfigObject, "activationThreshold", activationThreshold, errMsgSuffix)) {
        return false;
    }
    logger.logMsg(LOG_INFO, name + " activationThreshold=" + to_string(activationThreshold));

    if (!ConfigReader::getIntValue(patternConfigObject, "deactivationThreshold", deactivationThreshold, errMsgSuffix)) {
        deactivationThreshold = INT_MAX;
    }
    logger.logMsg(LOG_INFO, name + " deactivationThreshold=" + to_string(deactivationThreshold));

    if (!ConfigReader::getUnsignedIntValue(patternConfigObject, "numGoodMeasurementsForReactivation", numGoodMeasurementsForReactivation, errMsgSuffix)) {
        numGoodMeasurementsForReactivation = 0;
    }
    logger.logMsg(LOG_INFO, name + " numGoodMeasurementsForReactivation=" + to_string(numGoodMeasurementsForReactivation));
    // Start out with the good measurement count satisified.
    goodMeasurementCount = numGoodMeasurementsForReactivation;

    if (!ConfigReader::getIntValue(patternConfigObject, "densityScaledownFactor", densityScaledownFactor, errMsgSuffix, 1)) {
        return false;
    }
    logger.logMsg(LOG_INFO, name + " densityScaledownFactor=" + to_string(densityScaledownFactor));

    if (!ConfigReader::getFloatValue(patternConfigObject, "decayConstant", decayConstant)) {
        doAutoDecay = false;
        logger.logMsg(LOG_INFO, name + " auto decay is not enabled");
    }
    else {
        doAutoDecay = true;
        if (!ConfigReader::getUnsignedIntValue(patternConfigObject, "decayResetMs", decayResetMs, errMsgSuffix, 1)) {
            return false;
        }
        logger.logMsg(LOG_INFO, name + " decayConstant=" + to_string(decayConstant) + " decayResetMs=" + to_string(decayResetMs));
    }

    if (!ConfigReader::getUnsignedIntValue(patternConfigObject, "sparkleChangeIntervalMs", sparkleChangeIntervalMs, errMsgSuffix, 1)) {
        return false;
    }
    logger.logMsg(LOG_INFO, name + " sparkleChangeIntervalMs=" + to_string(sparkleChangeIntervalMs));

    if (!ConfigReader::getBoolValue(patternConfigObject, "useRandomColors", useRandomColors, errMsgSuffix)) {
        return false;
    }
    logger.logMsg(LOG_INFO, name + " useRandomColors=" + to_string(useRandomColors));

    if (!useRandomColors) {
        string rgbStr;
        if (!ConfigReader::getRgbPixelValue(patternConfigObject, "forwardSparkleColor", rgbStr, forwardSparkleColor, errMsgSuffix)) {
            return false;
        }
        logger.logMsg(LOG_INFO, name + " forwardSparkleColor=" + rgbStr);
        if (!ConfigReader::getRgbPixelValue(patternConfigObject, "reverseSparkleColor", rgbStr, reverseSparkleColor, errMsgSuffix)) {
            return false;
        }
        logger.logMsg(LOG_INFO, name + " reverseSparkleColor=" + rgbStr);
    }


    // ----- get input channel -----

    std::vector<Pattern::ChannelConfiguration> channelConfigs = getChannelConfigurations(widgets);
    if (channelConfigs.empty()) {
        logger.logMsg(LOG_ERR, "No valid widget channels are configured for " + name + ".");
        return false;
    }

    for (auto&& channelConfig : channelConfigs) {

        if (channelConfig.inputName == "density") {
            if (channelConfig.measurement == "velocity") {
                usePositionMeasurement = false;
            }
            else if (channelConfig.measurement == "position") {
                usePositionMeasurement = true;
            }
            else {
                logger.logMsg(LOG_ERR, channelConfig.inputName + " must specify position or velocity for " + name + ".");
                return false;
            }
            densityChannel = channelConfig.widgetChannel;
            logger.logMsg(LOG_INFO, name + " using " + channelConfig.widgetChannel->getName()
                             + (usePositionMeasurement ? " position measurement for " : " velocity measurement for ")
                             + channelConfig.inputName);

        }
        else {
            logger.logMsg(LOG_WARNING, "inputName '" + channelConfig.inputName
                + "' in input configuration for " + name + " is not recognized.");
            continue;
        }

    }

    return true;
}


void SparklePattern::setIsActive(bool nowActive, unsigned int nowMs)
{
    if (nowActive) {
        if (!isActive) {
            nextSparkleChangeMs = nowMs;
            // Reset the auto decay if we've been inactive long enough.
            if (doAutoDecay && (nowMs - inactiveStartMs >= decayResetMs || decayStartMs == 0)) {
                decayStartMs = nowMs;
            }
            inactiveStartMs = 0;
        }
    }
    else {
        if (inactiveStartMs == 0) {
            inactiveStartMs = nowMs;
        }
    }
    isActive = nowActive;
}


bool SparklePattern::update()
{
    // Don't do anything if no input channel was assigned.
    if (densityChannel == nullptr) {
        return false;
    }

    unsigned int nowMs = getNowMs();

    // If the widget channel has gone inactive, turn off this pattern.
    if (!densityChannel->getIsActive()) {
        setIsActive(false, nowMs);
        return false;
    }

    if ((usePositionMeasurement && densityChannel->getHasNewPositionMeasurement())
        || (!usePositionMeasurement && densityChannel->getHasNewVelocityMeasurement()))
    {
        int curMeasmt = usePositionMeasurement ? densityChannel->getPosition() : densityChannel->getVelocity();
        bool curMeasmtIsNegative = curMeasmt < 0;
        curMeasmt = abs(curMeasmt);

        // If the latest measurement is below the activation threshold, turn off this pattern.
        if (curMeasmt <= activationThreshold) {
            //logger.logMsg(LOG_DEBUG, to_string(curMeasmt) + " is below sparkle activation threshold " + to_string(activationThreshold));
            setIsActive(false, nowMs);
            return false;
        }

        // If the latest measurement is above the deactivation threshold, turn off this pattern.
        if (curMeasmt > deactivationThreshold) {
            //logger.logMsg(LOG_DEBUG, to_string(curMeasmt) + " is above sparkle deactivation threshold "
            //                  + to_string(deactivationThreshold));
            goodMeasurementCount = 0;
            setIsActive(false, nowMs);
            return false;
        }

        // If we haven't received enough good measurements after deactivation, stay inactive.
        // (This is a workaround for TriObelisk sending crap velocity data when a wheel is
        // spun counterclockwise.  The crap data cause the cone to frantically flash.  Although
        // pleasing to participants, the flashing dominates the cone and obscures everything else.)
        if (++goodMeasurementCount < numGoodMeasurementsForReactivation) {
            //logger.logMsg(LOG_DEBUG, to_string(goodMeasurementCount) + " of " + to_string(numGoodMeasurementsForReactivation)
            //                  + " good measurements received for reactivation.");
            setIsActive(false, nowMs);
            return false;
        }
        // It wouldn't overflow for, like, fucking forever, but we'll prevent
        // that from happening because defensive programming n' shit.
        goodMeasurementCount = numGoodMeasurementsForReactivation;

        setIsActive(true, nowMs);

        float decayFactor = 1.0;
        if (doAutoDecay) {
            float decaySeconds = (nowMs - decayStartMs) / 1000.0;
            decayFactor = expf(-1.0 * decayConstant * decaySeconds);
        }
        float sparkleFactor = std::min(((float) curMeasmt / (float) densityScaledownFactor) * decayFactor, (float) 1);

        numPixelsPerStringToSparkle = sparkleFactor * (float) pixelsPerString;
        motionIsReverse = curMeasmtIsNegative;

        //logger.logMsg(LOG_DEBUG, "curMeasmt=" + to_string(curMeasmt)
        //                  + ", decayFactor=" + to_string(decayFactor)
        //                  + ", sparkleFactor=" + to_string(sparkleFactor)
        //                  + ", numPixelsPerStringToSparkle=" + to_string(numPixelsPerStringToSparkle)
        //                  + ", motionIsReverse=" + to_string(motionIsReverse));
    }

    if (isActive && (int) (nowMs - nextSparkleChangeMs) >= 0) {
        nextSparkleChangeMs = nowMs + sparkleChangeIntervalMs;

        // Turn on approximately numPixelsPerStringToSparkle in each string
        // ("approximately" because we could select the same pixel twice).
        for (auto&& pixelString : pixelArray) {
            fillSolid(pixelString, RgbPixel::Black);
            for (int i = 0; i < numPixelsPerStringToSparkle; i++) {
                int randPos = random16(pixelsPerString);
                if (useRandomColors) {
                    pixelString[randPos].r = random8();
                    pixelString[randPos].g = random8();
                    pixelString[randPos].b = random8();
                }
                else {
                    pixelString[randPos] = motionIsReverse ? reverseSparkleColor : forwardSparkleColor;
                }
            }
        }
    }

    return isActive;
}

