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
#include "log.h"
#include "SwitchActivatedRegionsPattern.h"
#include "Pattern.h"
#include "Widget.h"
#include "WidgetChannel.h"


using namespace std;


SwitchActivatedRegionsPattern::SwitchActivatedRegionsPattern(const std::string& name)
    : IndicatorRegionsPattern(name, true)
{
}


bool SwitchActivatedRegionsPattern::initPattern(ConfigReader& config, std::map<WidgetId, Widget*>& widgets)
{
    if (!IndicatorRegionsPattern::initPattern(config, widgets)) {
        return false;
    }


    // ----- get pattern configuration -----

    auto patternConfig = config.getPatternConfigJsonObject(name);

    string errMsgSuffix = " in " + name + " pattern configuration.";


    // ----- get input channels -----

    std::vector<Pattern::ChannelConfiguration> channelConfigs = getChannelConfigurations(config, widgets);
    if (channelConfigs.empty()) {
        logMsg(LOG_ERR, "No valid widget channels are configured for " + name + ".");
        return false;
    }

    for (auto&& channelConfig : channelConfigs) {

        if (channelConfig.inputName == "switchArray") {
            switchArrayChannel = channelConfig.widgetChannel;
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


bool SwitchActivatedRegionsPattern::update()
{
    // Don't do anything if no input channel was assigned.
    if (switchArrayChannel == nullptr) {
        return false;
    }

//    unsigned int nowMs = getNowMs();

    // Let the regions do their animations.
    bool animationWantsDisplay = IndicatorRegionsPattern::update();

    if (!switchArrayChannel->getIsActive()) {
        //logMsg(LOG_DEBUG, "switchArrayChannel is inactive");
        if (!activeIndicators.empty()) {
            // If the widget has just gone inactive, turn off all the indicators.
            for (auto&& activeIndicator : activeIndicators) {
                activeIndicator->turnOffImmediately();
            }
            activeIndicators.clear();
        }
        isActive = false;
    }
    else if (switchArrayChannel->getHasNewPositionMeasurement()) {
        int measmt = switchArrayChannel->getPosition();
        bool anySwitchIsOn = false;
        for (unsigned int iSwitch = 0; iSwitch < 16 && iSwitch <= indicatorRegions.size(); ++iSwitch) {
            bool switchIsOn = measmt & (1 << iSwitch);
            IndicatorRegion* indicatorRegion = indicatorRegions[iSwitch];
            if (switchIsOn) {
                if (activeIndicators.find(indicatorRegion) == activeIndicators.end()) {
                    //logMsg(LOG_DEBUG, "switch " + to_string(iSwitch) + " turned on");
                    activeIndicators.insert(indicatorRegion);
                    indicatorRegion->makeAnimating(true);
                }
                anySwitchIsOn = true;
            }
            else {
                if (activeIndicators.find(indicatorRegion) != activeIndicators.end()) {
                    //logMsg(LOG_DEBUG, "switch " + to_string(iSwitch) + " turned off");
                    activeIndicators.erase(indicatorRegion);
                    indicatorRegion->turnOffImmediately();
                }
            }
        }
        isActive = anySwitchIsOn;
    }

    return isActive | animationWantsDisplay;
}

