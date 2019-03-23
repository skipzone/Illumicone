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

#pragma once

#include <map>
#include <string>
#include <vector>

#include "illumiconePixelTypes.h"
#include "illumiconeTypes.h"
#include "json11.hpp"
#include "WidgetChannel.h"
#include "WidgetId.h"


class Widget;


class Pattern
{
    public:

        Pattern(const std::string& name, bool usesHsvModel = false);
        virtual ~Pattern();

        Pattern() = delete;
        Pattern(const Pattern&) = delete;
        Pattern& operator =(const Pattern&) = delete;

        std::string getName() { return name; }

        bool init(const json11::Json& patternConfigObject,
                  const json11::Json& topLevelConfigObject,
                  std::map<WidgetId, Widget*>& widgets);

        virtual bool update() = 0;

        // configuration
        unsigned int pixelsPerString;
        unsigned int numStrings;
        bool usesHsvModel;
        int priority;
        int opacity;

        RgbConeStrings pixelArray;
        HsvConeStrings coneStrings;
        bool isActive;

    protected:

        struct ChannelConfiguration {
            std::string inputName;
            std::shared_ptr<WidgetChannel> widgetChannel;
            std::string measurement;
        };

        std::string name;
        json11::Json patternConfigObject;

        std::vector<ChannelConfiguration> getChannelConfigurations(std::map<WidgetId, Widget*>& widgets);

        virtual bool initPattern(std::map<WidgetId, Widget*>& widgets) = 0;
};

