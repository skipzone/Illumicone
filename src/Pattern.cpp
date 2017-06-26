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

#include <iostream>

#include "ConfigReader.h"
#include "json11.hpp"
#include "Pattern.h"

using namespace std;


Pattern::Pattern(const std::string name)
    : name(name)
{
}


std::vector<Pattern::ChannelConfiguration> Pattern::getChannelConfigurations(
    ConfigReader& config,
    std::map<WidgetId, Widget*>& widgets)
{
    std::vector<ChannelConfiguration> channelConfigs;

    auto patternConfig = config.getPatternConfigJsonObject(name);
    if (patternConfig["patternName"].string_value() != name) {
        cerr << name << " not found in patterns configuration section." << endl;
        return channelConfigs;
    }

    auto inputConfigs = patternConfig["inputs"].array_items();
    if (inputConfigs.empty()) {
        cerr << name << " inputs configuration is missing or empty:  " << patternConfig.dump() << endl;
        return channelConfigs;
    }

    for (auto& inputConfig : inputConfigs) {

        string inputName = inputConfig["inputName"].string_value();
        if (inputName.empty()) {
            cerr << name << " input configuration inputName is missing or empty:  " << inputConfig.dump() << endl;
            continue;
        }

        string widgetName = inputConfig["widgetName"].string_value();
        WidgetId widgetId = stringToWidgetId(widgetName);
        if (widgetId == WidgetId::invalid) {
            cerr << name << " input configuration for " << inputName
                << " does not specify a valid widget:  " << inputConfig.dump() << endl;
            continue;
        }

        if (widgets.find(widgetId) == widgets.end()) {
            cerr << name << " input configuration for " << inputName
                << " does not specify an available widget:  " << inputConfig.dump() << endl;
            continue;
        }
        Widget* widget = widgets[widgetId];

        if (!inputConfig["channelNumber"].is_number()) {
            cerr << name << " input configuration for " << inputName
                << " does not specify a channel number:  " << inputConfig.dump() << endl;
            continue;
        }
        unsigned int channelNumber = inputConfig["channelNumber"].int_value();
        shared_ptr<WidgetChannel> widgetChannel = widget->getChannel(channelNumber);
        if (widgetChannel == nullptr) {
            cerr << name << " input configuration for " << inputName << " specifies channel "
                << channelNumber << ", which doesn't exist:  " << inputConfig.dump() << endl;
            continue;
        }

        ChannelConfiguration channelConfig;
        channelConfig.inputName = inputName;
        channelConfig.widgetChannel = widgetChannel;
        channelConfig.measurement = inputConfig["measurement"].string_value();

        channelConfigs.emplace_back(channelConfig);
    }

    return channelConfigs;
}

