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

#include <string>
#include <vector>

#include "illumiconeTypes.h"
#include "json11.hpp"
#include "WidgetId.h"


class ConfigReader
{
    public:

        ConfigReader();
        virtual ~ConfigReader();

        ConfigReader(const ConfigReader&) = delete;
        ConfigReader& operator =(const ConfigReader&) = delete;

        bool readConfigurationFile(std::string fileName);

        std::string dumpToString();
        json11::Json getJsonObject();
        json11::Json getWidgetConfigJsonObject(WidgetId widgetId);
        json11::Json getPatternConfigJsonObject(const std::string& patternName);

        int getNumberOfStrings();
        int getNumberOfPixelsPerString();
        std::string getOpcServerIpAddress();
        std::string getPatconIpAddress();
        bool getSchedulePeriods(const std::string& scheduleName, std::vector<SchedulePeriod>& schedulePeriods);
        int getWidgetPortNumberBase();
        bool getWidgetGenerateSimulatedMeasurements(WidgetId widgetId);
        int getWidgetAutoInactiveMs(WidgetId widgetId);

    private:

        std::string configFileName;
        json11::Json configObj;

};
