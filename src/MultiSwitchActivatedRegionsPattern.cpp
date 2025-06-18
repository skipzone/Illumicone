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
#include "MultiSwitchActivatedRegionsPattern.h"
#include "Pattern.h"
#include "Widget.h"
#include "WidgetChannel.h"

using namespace std;


extern Log logger;


MultiSwitchActivatedRegionsPattern::MultiSwitchActivatedRegionsPattern(const std::string& name)
    : IndicatorRegionsPattern(name, true)   // usesHsvModel = true
{
}


bool MultiSwitchActivatedRegionsPattern::initPattern(std::map<WidgetId, Widget*>& widgets)
{
    if (!IndicatorRegionsPattern::initPattern(widgets)) {
        return false;
    }

    // ----- get pattern configuration -----

    string errMsgSuffix = " in " + name + " pattern configuration.";

    // ----- get input channels -----

    std::vector<Pattern::ChannelConfiguration> channelConfigs = getChannelConfigurations(widgets);
    if (channelConfigs.empty()) {
        logger.logMsg(LOG_ERR, "No valid widget channels are configured for " + name + ".");
        return false;
    }

    for (auto&& channelConfig : channelConfigs) {

        if (channelConfig.inputName == "multiSwitch") {
            // TODO:  warn if multiSwitchChannel already assigned
            multiSwitchChannel = channelConfig.widgetChannel;
        }
        else {
            logger.logMsg(LOG_WARNING, "inputName '" + channelConfig.inputName
                + "' in input configuration for " + name + " is not recognized.");
            continue;
        }
        logger.logMsg(LOG_INFO, name + " using " + channelConfig.widgetChannel->getName() + " for " + channelConfig.inputName);
    }

    return true;
}


bool MultiSwitchActivatedRegionsPattern::update()
{
    // Don't do anything if no input channel was assigned.
    if (multiSwitchChannel == nullptr) {
        return false;
    }

//    unsigned int nowMs = getNowMs();

    // Let the regions do their animations.
    bool animationWantsDisplay = IndicatorRegionsPattern::update();


/*
We can't use getIsActive because the switch squares can be independently active or inactive.
(Need to make sure that won't cause a problem in the widget classes.)  What we need to do is
use an array of switch states indexed by switchId.  The states are updated by iterating
through all new measurements (like Schroeder's does) in the order they were received, setting
switchStates[pos] = measmt.  Then, we interate over all the switch states, updating the
indicator region(s) for each switch.  If none of the switches are on, then we consider the
widget to be inactive, and we turn off all the indicators.

switchStates needs to be a struct that holds the current switch state and when the state was
set so that we can enforce timeouts.  When a switch times out (like, if someone is standing on
it forever), it isn't allowed to become active again until we get a measurement from it
indicating it is no longer activated.  Maybe use a state machine.

For now, let someone stand on a step for as long as they want.  Instead, we'll time out
a step when we haven't received an update from that step for more than a few seconds.
(When a step switch is deactivated, we should get at least a few measurements showing
it as not activated.  If the last thing we've heard from a step is that the switch was
activated, and we don't hear anything more, then the step has probably gone tits up or
we can't hear it anymore.)

 */

    // We'll process a limited number of messages so that we don't
    // completely block pattern updates and displays if the widget sends
    // a message storm.  The widget is considered to be active if any
    // switch is activated.
    isActive = false;
    unsigned int messageCount = 0;
    while (messageCount < maxMessagesPerPatternUpdate
           && multiSwitchChannel->getHasNewPositionMeasurement()
           && multiSwitchChannel->getHasNewVelocityMeasurement())
    {
        ++messageCount;

        unsigned int switchId = multiSwitchChannel->getPosition();
        bool switchIsActivated = multiSwitchChannel->getVelocity();

        ///logger.logMsg(LOG_DEBUG, "processing message #" + to_string(messageCount) + ", switchId=" + to_string(switchId) + " switchIsActivated=" + to_string(switchIsActivated));

        if (switchId < maxSwitches) {
            switchStates[switchId].isActivated = switchIsActivated;
            if (switchIsActivated) {
                isActive = true;
            }
        }
    }

/*
 * not sure we need to do something like this
    if (!multiSwitchChannel->getIsActive()) {
        logger.logMsg(LOG_DEBUG, "multiSwitchChannel is inactive");
        if (!activeIndicators.empty()) {
            // If the widget has just gone inactive, turn off all the indicators.
            for (auto&& activeIndicator : activeIndicators) {
                activeIndicator->turnOffImmediately();
            }
            activeIndicators.clear();
        }
        isActive = false;
    }
*/

    // TODO:  Add code to support fading indicators.  See the Midi... pattern.

    ///logger.logMsg(LOG_DEBUG, "maxSwitches=" + to_string(maxSwitches) + " indicatorRegions.size returns " + to_string(indicatorRegions.size()));
    for (unsigned int switchId = 0; switchId < maxSwitches; switchId++) {
        for (unsigned int indicatorIdx = 0; indicatorIdx < indicatorRegions.size(); indicatorIdx++) {
            ///logger.logMsg(LOG_DEBUG, "switchId=" + to_string(switchId) + " indicatorIdx=" + to_string(indicatorIdx));
            IndicatorRegion* indicatorRegion = indicatorRegions[indicatorIdx];
            ///logger.logMsg(LOG_DEBUG, "indicatorRegion switch id " + to_string(indicatorRegion->getSwitchId()));
            if (indicatorRegion->getSwitchId() == switchId) {
                ///logger.logMsg(LOG_DEBUG, "found indicator region for switch");
                if (switchStates[switchId].isActivated) {
                    ///logger.logMsg(LOG_DEBUG, "switchId " + to_string(switchId) + " is active");
                    if (activeIndicators.find(indicatorRegion) == activeIndicators.end()) {
                        ///logger.logMsg(LOG_DEBUG, "switch " + to_string(switchId) + " turned on");
                        activeIndicators.insert(indicatorRegion);
                        indicatorRegion->makeAnimating(true);
                    }
                }
                else {
                    if (activeIndicators.find(indicatorRegion) != activeIndicators.end()) {
                        ///logger.logMsg(LOG_DEBUG, "switch " + to_string(switchId) + " turned off");
                        activeIndicators.erase(indicatorRegion);
                        indicatorRegion->turnOffImmediately();
                    }
                }
            }
        }
    }

    return isActive | animationWantsDisplay;
}

