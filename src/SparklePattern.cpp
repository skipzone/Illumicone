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

#include <stdlib.h>

#include "ConfigReader.h"
#include "illumiconeUtility.h"
#include "illumiconePixelUtility.h"
#include "lib8tion.h"
#include "log.h"
#include "Pattern.h"
#include "SparklePattern.h"
#include "Widget.h"
#include "WidgetChannel.h"


using namespace std;


SparklePattern::SparklePattern(const std::string& name)
    : Pattern(name)
{
};


bool SparklePattern::initPattern(ConfigReader& config, std::map<WidgetId, Widget*>& widgets)
{
    nextSparkleChangeMs = 0;


    // ----- get pattern configuration -----

    string errMsgSuffix = " in " + name + " pattern configuration.";

    auto patternConfig = config.getPatternConfigJsonObject(name);

    if (!ConfigReader::getIntValue(patternConfig, "activationThreshold", activationThreshold, errMsgSuffix)) {
        return false;
    }
    logMsg(LOG_INFO, name + " activationThreshold=" + to_string(activationThreshold));

    if (!ConfigReader::getIntValue(patternConfig, "deactivationThreshold", deactivationThreshold, errMsgSuffix)) {
        deactivationThreshold = INT_MAX;
    }
    logMsg(LOG_INFO, name + " deactivationThreshold=" + to_string(deactivationThreshold));

    if (!ConfigReader::getUnsignedIntValue(patternConfig, "numGoodMeasurementsForReactivation", numGoodMeasurementsForReactivation, errMsgSuffix)) {
        numGoodMeasurementsForReactivation = 0;
    }
    logMsg(LOG_INFO, name + " numGoodMeasurementsForReactivation=" + to_string(numGoodMeasurementsForReactivation));
    // Start out with the good measurement count satisified.
    goodMeasurementCount = numGoodMeasurementsForReactivation;

    if (!ConfigReader::getIntValue(patternConfig, "densityScaledownFactor", densityScaledownFactor, errMsgSuffix, 1)) {
        return false;
    }
    logMsg(LOG_INFO, name + " densityScaledownFactor=" + to_string(densityScaledownFactor));

    if (!ConfigReader::getUnsignedIntValue(patternConfig, "sparkleChangeIntervalMs", sparkleChangeIntervalMs, errMsgSuffix, 1)) {
        return false;
    }
    logMsg(LOG_INFO, name + " sparkleChangeIntervalMs=" + to_string(sparkleChangeIntervalMs));

    if (!ConfigReader::getBoolValue(patternConfig, "useRandomColors", useRandomColors, errMsgSuffix)) {
        return false;
    }
    logMsg(LOG_INFO, name + " useRandomColors=" + to_string(useRandomColors));

    if (!useRandomColors) {
        string rgbStr;
        if (!ConfigReader::getRgbPixelValue(patternConfig, "forwardSparkleColor", rgbStr, forwardSparkleColor, errMsgSuffix)) {
            return false;
        }
        logMsg(LOG_INFO, name + " forwardSparkleColor=" + rgbStr);
        if (!ConfigReader::getRgbPixelValue(patternConfig, "reverseSparkleColor", rgbStr, reverseSparkleColor, errMsgSuffix)) {
            return false;
        }
        logMsg(LOG_INFO, name + " reverseSparkleColor=" + rgbStr);
    }


    // ----- get input channel -----

    std::vector<Pattern::ChannelConfiguration> channelConfigs = getChannelConfigurations(config, widgets);
    if (channelConfigs.empty()) {
        logMsg(LOG_ERR, "No valid widget channels are configured for " + name + ".");
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
                logMsg(LOG_ERR, channelConfig.inputName + " must specify position or velocity for " + name + ".");
                return false;
            }
            densityChannel = channelConfig.widgetChannel;
            logMsg(LOG_INFO, name + " using " + channelConfig.widgetChannel->getName()
                             + (usePositionMeasurement ? " position measurement for " : " velocity measurement for ")
                             + channelConfig.inputName);

        }
        else {
            logMsg(LOG_WARNING, "inputName '" + channelConfig.inputName
                + "' in input configuration for " + name + " is not recognized.");
            continue;
        }

    }

    return true;
}


bool SparklePattern::update()
{
    // Don't do anything if no input channel was assigned.
    if (densityChannel == nullptr) {
        return false;
    }

    // If the widget channel has gone inactive, turn off this pattern.
    if (!densityChannel->getIsActive()) {
        isActive = false;
        return false;
    }

    unsigned int nowMs = getNowMs();

    if ((usePositionMeasurement && densityChannel->getHasNewPositionMeasurement())
        || (!usePositionMeasurement && densityChannel->getHasNewVelocityMeasurement()))
    {
        int curMeasmt = usePositionMeasurement ? densityChannel->getPosition() : densityChannel->getVelocity();
        bool curMeasmtIsNegative = curMeasmt < 0;
        curMeasmt = abs(curMeasmt);

        // If the latest measurement is below the activation threshold, turn off this pattern.
        if (curMeasmt <= activationThreshold) {
            //logMsg(LOG_DEBUG, to_string(curMeasmt) + " is below sparkle activation threshold " + to_string(activationThreshold));
            isActive = false;
            return false;
        }

        // If the latest measurement is above the deactivation threshold, turn off this pattern.
        if (curMeasmt > deactivationThreshold) {
            //logMsg(LOG_DEBUG, to_string(curMeasmt) + " is above sparkle deactivation threshold "
            //                  + to_string(deactivationThreshold));
            goodMeasurementCount = 0;
            isActive = false;
            return false;
        }

        // If we haven't received enough good measurements after deactivation, stay inactive.
        // (This is a workaround for TriObelisk sending crap velocity data when a wheel is
        // spun counterclockwise.  The crap data cause the cone to frantically flash.  Although
        // pleasing to participants, the flashing dominates the cone and obscures everything else.)
        if (++goodMeasurementCount < numGoodMeasurementsForReactivation) {
            //logMsg(LOG_DEBUG, to_string(goodMeasurementCount) + " of " + to_string(numGoodMeasurementsForReactivation)
            //                  + " good measurements received for reactivation.");
            isActive = false;
            return false;
        }
        // It wouldn't overflow for, like, fucking forever, but we'll prevent
        // that from happening because defensive programming n' shit.
        goodMeasurementCount = numGoodMeasurementsForReactivation;

        if (!isActive) {
            isActive = true;
            nextSparkleChangeMs = nowMs;
        }

        float sparkePercentage = min((float) curMeasmt / (float) densityScaledownFactor, (float) 1);
        numPixelsPerStringToSparkle = sparkePercentage * (float) pixelsPerString;
        motionIsReverse = curMeasmtIsNegative;

        //logMsg(LOG_DEBUG, "curMeasmt=" + to_string(curMeasmt)
        //                  + ", sparkePercentage=" + to_string(sparkePercentage)
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

