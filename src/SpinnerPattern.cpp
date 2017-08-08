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
#include "SpinnerPattern.h"
#include "Pattern.h"
#include "Widget.h"
#include "WidgetChannel.h"


using namespace std;


SpinnerPattern::SpinnerPattern(const std::string& name)
    : Pattern(name)
{
}


bool SpinnerPattern::initPattern(ConfigReader& config, std::map<WidgetId, Widget*>& widgets)
{
    if (!IndicatorRegionsPattern::initPattern(ConfigReader& config, std::map<WidgetId, Widget*>& widgets)) {
        return false;
    }

j
    // ----- get pattern configuration -----

    auto patternConfig = config.getPatternConfigJsonObject(name);

    if (!patternConfig["persistenceMs"].is_number()) {
        logMsg(LOG_ERR, "persistenceMs not specified in " + name + " pattern configuration.");
        return false;
    }
    persistenceMs = patternConfig["persistenceMs"].int_value();
    logMsg(LOG_INFO, name + " persistenceMs=" + to_string(persistenceMs));


    // ----- get input channels -----

    std::vector<Pattern::ChannelConfiguration> channelConfigs = getChannelConfigurations(config, widgets);
    if (channelConfigs.empty()) {
        logMsg(LOG_ERR, "No valid widget channels are configured for " + name + ".");
        return false;
    }

    for (auto&& channelConfig : channelConfigs) {

        if (channelConfig.inputName == "spinnerPosition") {
            spinnerPositionChannel = channelConfig.widgetChannel;
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


bool SpinnerPattern::update()
{
    // Don't do anything if no input channel was assigned.
    if (spinnerPositionChannel == nullptr) {
        return false;
    }

    unsigned int nowMs = getNowMs();

    if (!spinnerPositionChannel->getIsActive()) {
        //logMsg(LOG_DEBUG, "spinnerPositionChannel is inactive");
        // TODO:  =-=-=-=-= wait for persistenceMs before going inactive
        isActive = false;
        return isActive;
    }

    return isActive;
}

