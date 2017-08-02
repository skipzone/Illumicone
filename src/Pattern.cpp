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

#include "colorutils.h"
#include "ConfigReader.h"
#include "illumiconePixelUtility.h"
#include "json11.hpp"
#include "log.h"
#include "Pattern.h"

using namespace std;


Pattern::Pattern(const std::string& name, bool usesHsvModel)
    : usesHsvModel(usesHsvModel)
    , name(name)
{
}


Pattern::~Pattern()
{
    if (usesHsvModel) {
        freeConePixels<HsvConeStrings, HsvPixel>(coneStrings);
    }
    else {
        freeConePixels<RgbConeStrings, RgbPixel>(pixelArray);
    }
}


bool Pattern::goInactive()
{
    // If we're just now going inactive, we need to return true
    // so that this pattern can be cleared from display.
    bool retval = isActive;

    if (isActive) {
        isActive = false;
        // Set all the pixels to 0 intensity to make this pattern effectively transparent.
        if (usesHsvModel) {
            clearAllPixels(coneStrings);
        }
        else {
            clearAllPixels(pixelArray);
        }
    }

    return retval;
}


bool Pattern::init(ConfigReader& config, std::map<WidgetId, Widget*>& widgets)
{
    numStrings = config.getNumberOfStrings();
    pixelsPerString = config.getNumberOfPixelsPerString();

    if (usesHsvModel) {
        allocateConePixels<HsvConeStrings, HsvPixelString, HsvPixel>(coneStrings, numStrings, pixelsPerString);
    }
    else {
        allocateConePixels<RgbConeStrings, RgbPixelString, RgbPixel>(pixelArray, numStrings, pixelsPerString);
    }

    auto patternConfig = config.getPatternConfigJsonObject(name);

    if (!patternConfig["priority"].is_number()) {
        logMsg(LOG_ERR, "priority not specified in " + name + " pattern configuration.");
        return false;
    }
    priority = patternConfig["priority"].int_value();
    logMsg(LOG_INFO, name + " priority=" + to_string(priority));

    if (!patternConfig["opacity"].is_number()) {
        logMsg(LOG_ERR, "opacity not specified in " + name + " pattern configuration.");
        return false;
    }
    opacity = patternConfig["opacity"].int_value();
    logMsg(LOG_INFO, name + " opacity=" + to_string(opacity));

    return initPattern(config, widgets);
}


std::vector<Pattern::ChannelConfiguration> Pattern::getChannelConfigurations(
    ConfigReader& config,
    std::map<WidgetId, Widget*>& widgets)
{
    std::vector<ChannelConfiguration> channelConfigs;

    auto patternConfig = config.getPatternConfigJsonObject(name);
    if (patternConfig["name"].string_value() != name) {
        logMsg(LOG_ERR, name + " not found in patterns configuration section.");
        return channelConfigs;
    }

    auto inputConfigs = patternConfig["inputs"].array_items();
    if (inputConfigs.empty()) {
        logMsg(LOG_ERR, name + " inputs configuration is missing or empty:  " + patternConfig.dump());
        return channelConfigs;
    }

    for (auto& inputConfig : inputConfigs) {

        string inputName = inputConfig["inputName"].string_value();
        if (inputName.empty()) {
            logMsg(LOG_ERR, name + " input configuration inputName is missing or empty:  " + inputConfig.dump());
            continue;
        }

        string widgetName = inputConfig["widgetName"].string_value();
        WidgetId widgetId = stringToWidgetId(widgetName);
        if (widgetId == WidgetId::invalid) {
            logMsg(LOG_ERR, name + " input configuration for " + inputName
                + " does not specify a valid widget:  " + inputConfig.dump());
            continue;
        }

        if (widgets.find(widgetId) == widgets.end()) {
            logMsg(LOG_ERR, name + " input configuration for " + inputName
                + " does not specify an available widget:  " + inputConfig.dump());
            continue;
        }
        Widget* widget = widgets[widgetId];

        if (!inputConfig["channelNumber"].is_number()) {
            logMsg(LOG_ERR, name + " input configuration for " + inputName
                + " does not specify a channel number:  " + inputConfig.dump());
            continue;
        }
        unsigned int channelNumber = inputConfig["channelNumber"].int_value();
        shared_ptr<WidgetChannel> widgetChannel = widget->getChannel(channelNumber);
        if (widgetChannel == nullptr) {
            logMsg(LOG_ERR, name + " input configuration for " + inputName + " specifies channel "
                + to_string(channelNumber) + ", which doesn't exist:  " + inputConfig.dump());
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

