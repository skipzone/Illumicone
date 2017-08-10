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

#include <climits>
#include <string>
#include <vector>

#include "illumiconeTypes.h"
#include "json11.hpp"
#include "WidgetId.h"


class ConfigReader
{
    public:

        static bool getBoolValue(const json11::Json& jsonObj,
                                 const std::string& name,
                                 bool& value,
                                 const std::string& errorMessageSuffix);

        static bool getIntValue(const json11::Json& jsonObj,
                                const std::string& name,
                                int& value,
                                const std::string& errorMessageSuffix,
                                int minValue = INT_MIN,
                                int maxValue = INT_MAX);

        static bool getStringValue(const json11::Json& jsonObj,
                                   const std::string& name,
                                   std::string& value,
                                   const std::string& errorMessageSuffix,
                                   bool allowEmptyString = false);

        static bool getUnsignedIntValue(const json11::Json& jsonObj,
                                        const std::string& name,
                                        unsigned int& value,
                                        const std::string& errorMessageSuffix,
                                        unsigned int minValue = 0,
                                        unsigned int maxValue = UINT_MAX);

        ConfigReader();
        virtual ~ConfigReader();

        ConfigReader(const ConfigReader&) = delete;
        ConfigReader& operator =(const ConfigReader&) = delete;

        bool readConfigurationFile(std::string fileName);

        std::string dumpToString();
        json11::Json getJsonObject();
        json11::Json getWidgetConfigJsonObject(const std::string& widgetName);
        json11::Json getPatternConfigJsonObject(const std::string& patternName);

        int getNumberOfStrings();
        int getNumberOfPixelsPerString();
        std::string getOpcServerIpAddress();
        std::string getPatconIpAddress();
        std::string getPatternBlendMethod();
        unsigned int getPatternRunLoopSleepIntervalUs();
        bool getSchedulePeriods(const std::string& scheduleName, std::vector<SchedulePeriod>& schedulePeriods);
        int getWidgetPortNumberBase();

        bool getWidgetGenerateSimulatedMeasurements(const std::string& widgetName);
        int getWidgetAutoInactiveMs(const std::string& widgetName);

    private:

        std::string configFileName;
        json11::Json configObj;

};

