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
#include <iostream>

#include "ConfigReader.h"
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
    auto patternConfig = config.getPatternConfigJsonObject(name);

    if (!patternConfig["activationThreshold"].is_number()) {
        logMsg(LOG_ERR, "activationThreshold not specified in " + name + " pattern configuration.");
        return false;
    }
    activationThreshold = patternConfig["activationThreshold"].int_value();
    logMsg(LOG_INFO, name + " activationThreshold=" + to_string(activationThreshold));

    if (!patternConfig["densityScaledownFactor"].is_number()) {
        logMsg(LOG_ERR, "densityScaledownFactor not specified in " + name + " pattern configuration.");
        return false;
    }
    densityScaledownFactor = patternConfig["densityScaledownFactor"].int_value();
    if (densityScaledownFactor == 0) {
        logMsg(LOG_ERR, "densityScaledownFactor is zero in " + name + " pattern configuration.");
        return false;
    }
    logMsg(LOG_INFO, name + " densityScaledownFactor=" + to_string(densityScaledownFactor));

    std::vector<Pattern::ChannelConfiguration> channelConfigs = getChannelConfigurations(config, widgets);
    if (channelConfigs.empty()) {
        logMsg(LOG_ERR, "No valid widget channels are configured for " + name + ".");
        return false;
    }

    for (auto&& channelConfig : channelConfigs) {

        if (channelConfig.inputName == "density") {
            densityChannel = channelConfig.widgetChannel;
        }
        else {
            logMsg(LOG_WARNING, "Warning:  inputName '" + channelConfig.inputName
                + "' in input configuration for " + name + " is not recognized.");
            continue;
        }
        logMsg(LOG_INFO, name + " using " + channelConfig.widgetChannel->getName() + " for " + channelConfig.inputName);

        if (channelConfig.measurement != "velocity") {
            logMsg(LOG_WARNING, "Warning:  " + name + " supports only velocity measurements, but the input configuration for "
                + channelConfig.inputName + " doesn't specify velocity.");
        }
    }

    return true;
}


void SparklePattern::goInactive()
{
    if (isActive) {
        isActive = false;
        // Set all the pixels to 0 intensity to make this pattern effectively transparent.
        for (auto&& pixels:pixelArray) {
            for (auto&& pixel:pixels) {
                pixel.r = 0;
                pixel.g = 0;
                pixel.b = 0;
            }
        }
    }
}


bool SparklePattern::update()
{
    // TODO:  The frequency of the sparkling is determined by the frequency at which the widget
    //        sends measurements. Instead, the frequency should be determined here in the pattern.

    // Don't do anything if no input channel was assigned.
    if (densityChannel == nullptr) {
        return false;
    }

    // If the widget channel has gone inactive, turn off this pattern.
    if (!densityChannel->getIsActive()) {
        goInactive();
        return false;
    }

    // No change to the pattern if we haven't received a new measurement.
    if (!densityChannel->getHasNewVelocityMeasurement()) {
        return isActive;
    }

    int curVel = densityChannel->getVelocity();

    // If the latest measurement is below the activation threshold, turn off this pattern.
    if (curVel <= activationThreshold) {
        //cout << curVel << " is below sparkle activation threshold " << activationThreshold << endl;
        goInactive();
        return false;
    }

    isActive = true;

    float sparkePercentage = min((float) curVel / (float) densityScaledownFactor, (float) 1);
    int numPixelsPerStringToSparkle = sparkePercentage * (float) pixelsPerString;

    //cout << "curVel: " << curVel << endl;
    //cout << "sparkePercentage: " << sparkePercentage << endl;
    //cout << "numPixelsPerStringToSparkle: " << numPixelsPerStringToSparkle << endl;

    // Set approximately numPixelsPerStringToSparkle to a random color in each string.
    // ("Approximately" because we could select the same pixel twice.)
    for (auto&& pixels:pixelArray) {
        for (auto&& pixel:pixels) {
            pixel.r = 0;
            pixel.g = 0;
            pixel.b = 0;
        }
        for (int i = 0; i < numPixelsPerStringToSparkle; i++) {
            int randPos = rand() % pixelsPerString;
            pixels[randPos].r = rand() % 255;
            pixels[randPos].g = rand() % 255;
            pixels[randPos].b = rand() % 255;
        }
    }

    return true;
}

