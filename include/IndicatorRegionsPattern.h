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

#include <vector>

#include "Pattern.h"


class ConfigReader;
class IndicatorRegion;
class Widget;


class IndicatorRegionsPattern : public Pattern {

    public:

        IndicatorRegionsPattern(const std::string& name, bool usesHsvModel = false);
        virtual ~IndicatorRegionsPattern();

        IndicatorRegionsPattern() = delete;
        IndicatorRegionsPattern(const IndicatorRegionsPattern&) = delete;
        IndicatorRegionsPattern& operator =(const IndicatorRegionsPattern&) = delete;

        virtual bool update();

    protected:

        virtual bool initPattern(ConfigReader& config, std::map<WidgetId, Widget*>& widgets);

        std::vector<IndicatorRegion*> indicatorRegions;

    private:

};

