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

#include <string>

#include <stdlib.h>

#include "ConfigReader.h"
#include "illumiconeUtility.h"
#include "IndicatorRegion.h"
#include "Log.h"
#include "SpinnerPattern.h"
#include "Pattern.h"
#include "Widget.h"
#include "WidgetChannel.h"

using namespace std;


extern Log logger;


SpinnerPattern::SpinnerPattern(const std::string& name)
    : IndicatorRegionsPattern(name, true)
    , activeIndicator(nullptr)
{
}


bool SpinnerPattern::initPattern(ConfigReader& config, std::map<WidgetId, Widget*>& widgets)
{
    if (!IndicatorRegionsPattern::initPattern(config, widgets)) {
        return false;
    }


    // ----- get pattern configuration -----

    auto patternConfig = config.getPatternConfigJsonObject(name);

    string errMsgSuffix = " in " + name + " pattern configuration.";

    if (!ConfigReader::getUnsignedIntValue(patternConfig,
                                           "selectedBlockAnimationIntervalMs",
                                           selectedBlockAnimationIntervalMs,
                                           errMsgSuffix)) {
        return false;
    }


    // ----- get input channels -----

    std::vector<Pattern::ChannelConfiguration> channelConfigs = getChannelConfigurations(config, widgets);
    if (channelConfigs.empty()) {
        logger.logMsg(LOG_ERR, "No valid widget channels are configured for " + name + ".");
        return false;
    }

    for (auto&& channelConfig : channelConfigs) {

        if (channelConfig.inputName == "spinnerPosition") {
            spinnerPositionChannel = channelConfig.widgetChannel;
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


bool SpinnerPattern::update()
{
    // Don't do anything if no input channel was assigned.
    if (spinnerPositionChannel == nullptr) {
        return false;
    }

    unsigned int nowMs = getNowMs();

    // Let the regions do their animations.
    bool animationWantsDisplay = IndicatorRegionsPattern::update();

    if (!spinnerPositionChannel->getIsActive()) {
        //logger.logMsg(LOG_DEBUG, "spinnerPositionChannel is inactive");
        if (activeIndicator != nullptr) {
            // If the widget has just gone inactive, make the final (selected) block animate for a while.
            if (!activeIndicator->getIsAnimating()) {
                activeIndicator->makeAnimating(true);
                stopAnimatingMs = nowMs + selectedBlockAnimationIntervalMs;
            }
            else if ((int) (nowMs - stopAnimatingMs) >= 0) {
                activeIndicator->turnOffImmediately();
                activeIndicator = nullptr;
            }
        }
        else {
            isActive = false;
        }
    }
    else if (spinnerPositionChannel->getHasNewPositionMeasurement()) {
        if (activeIndicator != nullptr) {
            activeIndicator->turnOffImmediately();
            activeIndicator = nullptr;
        }
        unsigned int indicatorIdx = spinnerPositionChannel->getPosition() % indicatorRegions.size();
        //logger.logMsg(LOG_DEBUG, "indicatorIdx = " + to_string(indicatorIdx));
        activeIndicator = indicatorRegions[indicatorIdx];
        activeIndicator->turnOnImmediately();
        isActive = true;
    }

    return isActive | animationWantsDisplay;
}

