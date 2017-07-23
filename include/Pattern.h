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

#ifndef PATTERN_H
#define PATTERN_H

#include <map>
#include <string>
#include <vector>

#include "illumiconeTypes.h"
#include "Widget.h"


class Pattern
{
    public:

        Pattern(const std::string& name);
        virtual ~Pattern() {}

        Pattern() = delete;
        Pattern(const Pattern&) = delete;
        Pattern& operator =(const Pattern&) = delete;

        std::string getName() { return name; }

        bool init(ConfigReader& config, std::map<WidgetId, Widget*>& widgets);
        virtual bool update() = 0;

        int pixelsPerString;
        int numStrings;
        std::vector<std::vector<opc_pixel_t>> pixelArray;

        int priority;
        int opacity;
        std::string name;
        bool isActive;


    protected:

        struct ChannelConfiguration {
            std::string inputName;
            std::shared_ptr<WidgetChannel> widgetChannel;
            std::string measurement;
        };

        std::vector<ChannelConfiguration> getChannelConfigurations(ConfigReader& config, std::map<WidgetId, Widget*>& widgets);

        virtual bool initPattern(ConfigReader& config, std::map<WidgetId, Widget*>& widgets) = 0;
};

#endif /* PATTERN_H */
