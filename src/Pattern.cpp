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
#include "Log.h"
#include "Pattern.h"
#include "Widget.h"


using namespace std;


extern Log logger;


Pattern::Pattern(const std::string& name, bool usesHsvModel)
    : usesHsvModel(usesHsvModel)
    , isActive(false)
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


bool Pattern::init(const json11::Json& patternConfigObject,
                   const json11::Json& topLevelConfigObject,
                   std::map<WidgetId, Widget*>& widgets)
{
    this->patternConfigObject = patternConfigObject;

    string logMsgSuffix = " for pattern " + name;

    if (!ConfigReader::getUnsignedIntValue(topLevelConfigObject, "numberOfStrings", numStrings,
                                           logMsgSuffix, 8, 128)   // nothing magical about 8 and 128, just reasonable
        || !ConfigReader::getUnsignedIntValue(topLevelConfigObject, "numberOfPixelsPerString", pixelsPerString,
                                           logMsgSuffix, 8, 512))   // nothing magical about 8 and 512, just reasonable
    {
        return false;
    }

    if (usesHsvModel) {
        allocateConePixels<HsvConeStrings, HsvPixelString, HsvPixel>(coneStrings, numStrings, pixelsPerString);
    }
    else {
        allocateConePixels<RgbConeStrings, RgbPixelString, RgbPixel>(pixelArray, numStrings, pixelsPerString);
    }

    if (!ConfigReader::getIntValue(patternConfigObject, "priority", priority, logMsgSuffix)) return false;
    logger.logMsg(LOG_INFO, name + " priority=" + to_string(priority));

    if (!ConfigReader::getIntValue(patternConfigObject, "opacity", opacity, logMsgSuffix)) return false;
    logger.logMsg(LOG_INFO, name + " opacity=" + to_string(opacity));

    return initPattern(widgets);
}


std::vector<Pattern::ChannelConfiguration> Pattern::getChannelConfigurations(std::map<WidgetId, Widget*>& widgets)
{
    std::vector<ChannelConfiguration> channelConfigs;

    // TODO:  modify to use ConfigReader::get...

    auto inputConfigs = patternConfigObject["inputs"].array_items();
    if (inputConfigs.empty()) {
        logger.logMsg(LOG_ERR, name + " inputs configuration is missing or empty:  " + patternConfigObject.dump());
        return channelConfigs;
    }

    for (auto& inputConfig : inputConfigs) {

        string inputName = inputConfig["inputName"].string_value();
        if (inputName.empty()) {
            logger.logMsg(LOG_ERR, name + " input configuration inputName is missing or empty:  " + inputConfig.dump());
            continue;
        }

        string widgetName = inputConfig["widgetName"].string_value();
        WidgetId widgetId = stringToWidgetId(widgetName);
        if (widgetId == WidgetId::invalid) {
            logger.logMsg(LOG_ERR, name + " input configuration for " + inputName
                + " does not specify a valid widget:  " + inputConfig.dump());
            continue;
        }

        if (widgets.find(widgetId) == widgets.end()) {
            logger.logMsg(LOG_ERR, name + " input configuration for " + inputName
                + " does not specify an available widget:  " + inputConfig.dump());
            continue;
        }
        Widget* widget = widgets[widgetId];

        if (!inputConfig["channelNumber"].is_number()) {
            logger.logMsg(LOG_ERR, name + " input configuration for " + inputName
                + " does not specify a channel number:  " + inputConfig.dump());
            continue;
        }
        unsigned int channelNumber = inputConfig["channelNumber"].int_value();
        shared_ptr<WidgetChannel> widgetChannel = widget->getChannel(channelNumber);
        if (widgetChannel == nullptr) {
            logger.logMsg(LOG_ERR, name + " input configuration for " + inputName + " specifies channel "
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

