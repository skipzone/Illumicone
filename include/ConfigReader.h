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
#include <limits>
#include <string>
#include <vector>

#include "illumiconePixelTypes.h"
#include "illumiconeTypes.h"
#include "json11.hpp"
#include "pixeltypes.h"
#include "WidgetId.h"


class ConfigReader
{
    public:

        static bool getBoolValue(const json11::Json& jsonObj,
                                 const std::string& name,
                                 bool& value,
                                 const std::string& errorMessageSuffix = "");

        static bool getDoubleValue(const json11::Json& jsonObj,
                                   const std::string& name,
                                   double& value,
                                   const std::string& errorMessageSuffix = "",
                                   double minValue = std::numeric_limits<double>::lowest(),
                                   double maxValue = std::numeric_limits<double>::max());

        static bool getFloatValue(const json11::Json& jsonObj,
                                  const std::string& name,
                                  float& value,
                                  const std::string& errorMessageSuffix = "",
                                  float minValue = std::numeric_limits<float>::lowest(),
                                  float maxValue = std::numeric_limits<float>::max());

        static bool getHsvPixelValue(const json11::Json& jsonObj,
                                     const std::string& name,
                                     std::string& rgbStr,
                                     HsvPixel& value,
                                     const std::string& errorMessageSuffix = "",
                                     bool allowEmptyString = false,
                                     const HsvPixel& defaultValue = HsvPixel(0, 0, 0));

        static bool getJsonObject(const json11::Json& jsonObj,
                                  const std::string& name,
                                  json11::Json& value,
                                  const std::string& errorMessageSuffix = "");

        static bool getIntValue(const json11::Json& jsonObj,
                                const std::string& name,
                                int& value,
                                const std::string& errorMessageSuffix = "",
                                int minValue = INT_MIN,
                                int maxValue = INT_MAX);

        static bool getRgbPixelValue(const json11::Json& jsonObj,
                                     const std::string& name,
                                     std::string& rgbStr,
                                     RgbPixel& value,
                                     const std::string& errorMessageSuffix = "",
                                     bool allowEmptyString = false,
                                     const RgbPixel& defaultValue = CRGB::Black);

        static bool getSchedulePeriods(const json11::Json& jsonObj,
                                       const std::string& scheduleName,
                                       std::vector<SchedulePeriod>& schedulePeriods);

        static bool getStringValue(const json11::Json& jsonObj,
                                   const std::string& name,
                                   std::string& value,
                                   const std::string& errorMessageSuffix = "",
                                   bool allowEmptyString = false);

        static bool getUnsignedIntValue(const json11::Json& jsonObj,
                                        const std::string& name,
                                        unsigned int& value,
                                        const std::string& errorMessageSuffix = "",
                                        unsigned int minValue = 0,
                                        unsigned int maxValue = UINT_MAX);

        static json11::Json mergeConfigObjects(const json11::Json& primaryJsonObj, const json11::Json& secondaryJsonObj);


        ConfigReader();
        virtual ~ConfigReader();

        ConfigReader(const ConfigReader&) = delete;
        ConfigReader& operator =(const ConfigReader&) = delete;

        bool readConfigurationFile(std::string fileName);

        std::string dumpToString();
        json11::Json getConfigObject();
        json11::Json getWidgetConfigJsonObject(const std::string& widgetName);
        json11::Json getPatternConfigJsonObject(const std::string& patternName);

        std::string getPatconIpAddress();
        unsigned int getRadioPollingLoopSleepIntervalUs();
        int getWidgetPortNumberBase();

    private:

        std::string configFileName;
        json11::Json configObj;

};

