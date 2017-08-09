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

// TODO 7/31/2017 ross:  Use logMsg in place of cerr.

#include <fstream>
#include <iostream>
#include <sstream>

#include "ConfigReader.h"
#include "illumiconeTypes.h"
#include "log.h"

using namespace std;
using namespace json11;


bool ConfigReader::getBoolValue(const json11::Json& jsonObj,
                                const std::string& name,
                                bool& value,
                                const std::string& errorMessageSuffix)
{
    if (!jsonObj[name].is_bool()) {
        logMsg(LOG_ERR, name + " is not present or is not a boolean value" + errorMessageSuffix);
        return false;
    }
    value = jsonObj[name].bool_value();
    return true;
}


bool ConfigReader::getIntValue(const json11::Json& jsonObj,
                               const std::string& name,
                               int& value,
                               const std::string& errorMessageSuffix,
                               int minValue,
                               int maxValue)
{
    if (!jsonObj[name].is_number()) {
        logMsg(LOG_ERR, name + " is not present or is not an integer" + errorMessageSuffix);
        return false;
    }
    value = jsonObj[name].int_value();
    if (value < minValue || value > maxValue) {
        logMsg(LOG_ERR, name + " is outside of range [" + to_string(minValue)
                        + ", " + to_string(maxValue) + "]" + errorMessageSuffix);
        return false;
    }
    return true;
}


bool ConfigReader::getStringValue(const json11::Json& jsonObj,
                                  const std::string& name,
                                  string& value,
                                  const std::string& errorMessageSuffix,
                                  bool allowEmptyString)
{
    if (!jsonObj[name].is_string()) {
        logMsg(LOG_ERR, name + " is not present or is not a string" + errorMessageSuffix);
        return false;
    }
    value = jsonObj[name].string_value();
    if (!allowEmptyString && value.empty()) {
        logMsg(LOG_ERR, name + " is empty" + errorMessageSuffix);
        return false;
    }
    return true;
}


bool ConfigReader::getUnsignedIntValue(const json11::Json& jsonObj,
                                       const std::string& name,
                                       unsigned int& value,
                                       const std::string& errorMessageSuffix,
                                       int minValue,
                                       int maxValue)
{
    int i;
    bool retval = getIntValue(jsonObj, name, i, errorMessageSuffix, minValue, maxValue);
    value = i;
    return retval;
}


ConfigReader::ConfigReader()
{
}


ConfigReader::~ConfigReader()
{
}


bool ConfigReader::readConfigurationFile(std::string fileName)
{
    configFileName = fileName;

    ifstream configFile(configFileName, ios_base::in);
    if (!configFile.is_open()) {
        cerr << "Can't open " << configFileName << endl;
        return false;
    }
    stringstream jsonSstr;
    jsonSstr << configFile.rdbuf();
    configFile.close();

    string err;
    configObj = Json::parse(jsonSstr.str(), err, JsonParse::COMMENTS);
    if (!err.empty()) {
        cerr << "Parse of " << configFileName << " failed:  " << err << endl;
        return false;
    }

    // TODO 6/12/2017 ross:  Build a map associating widget ids with widget config objects and use it to access the objects.

    return true;
}


string ConfigReader::dumpToString()
{
    return configObj.dump();
}


Json ConfigReader::getJsonObject()
{
    return configObj;
}


Json ConfigReader::getWidgetConfigJsonObject(const std::string& widgetName)
{
    for (auto& widgetConfigObj : configObj["widgets"].array_items()) {
        if (widgetConfigObj["name"].string_value() == widgetName) {
            return widgetConfigObj;
        }
    }
    return Json("{}");
}


Json ConfigReader::getPatternConfigJsonObject(const string& patternName)
{
    for (auto& patternConfigObj : configObj["patterns"].array_items()) {
        if (patternConfigObj["name"].string_value() == patternName) {
            return patternConfigObj;
        }
    }
    return Json("{}");
}


int ConfigReader::getNumberOfStrings()
{
    return configObj["numberOfStrings"].int_value();
}


int ConfigReader::getNumberOfPixelsPerString()
{
    return configObj["numberOfPixelsPerString"].int_value();
}


string ConfigReader::getOpcServerIpAddress()
{
    return configObj["opcServerIpAddress"].string_value();
}


string ConfigReader::getPatconIpAddress()
{
    if (!configObj["patconIpAddress"].is_string()) {
        logMsg(LOG_ERR, "patconIpAddress missing from configuration.");
        return "";
    }
    return configObj["patconIpAddress"].string_value();
}


string ConfigReader::getPatternBlendMethod()
{
    if (!configObj["patternBlendMethod"].is_string()) {
        logMsg(LOG_ERR, "patternBlendMethod missing from configuration.");
        return "";
    }
    return configObj["patternBlendMethod"].string_value();
}


bool ConfigReader::getSchedulePeriods(const std::string& scheduleName, std::vector<SchedulePeriod>& schedulePeriods)
{
    bool problemEncountered = false;

    for (auto& periodConfigObj : configObj[scheduleName].array_items()) {

        string desc = periodConfigObj["description"].string_value();
        if (desc.empty()) {
            cerr << "Scheduled period has no description:  " << periodConfigObj.dump() << endl;
            problemEncountered = true;
            continue;
        }
        string startTimeStr = periodConfigObj["startDateTime"].string_value();
        if (startTimeStr.empty()) {
            cerr << "Scheduled period has no startDateTime:  " << periodConfigObj.dump() << endl;
            problemEncountered = true;
            continue;
        }
        string endTimeStr = periodConfigObj["endDateTime"].string_value();
        if (endTimeStr.empty()) {
            cerr << "Scheduled period has no endDateTime:  " << periodConfigObj.dump() << endl;
            problemEncountered = true;
            continue;
        }

        bool isDaily;
        string dateTimeFormat;
        if (startTimeStr.length() == 8) {
            isDaily = true;
            dateTimeFormat = "%H:%M:%S";
        }
        else {
            isDaily = false;
            dateTimeFormat = "%Y-%m-%d %H:%M:%S";
        }

        struct tm tmTime;
        char* strptimeRetVal;
        time_t now;
        time(&now);

        localtime_r(&now, &tmTime);
        strptimeRetVal = strptime(startTimeStr.c_str(), dateTimeFormat.c_str(), &tmTime);
        if (strptimeRetVal == nullptr) {
            cerr << "Unable to parse startDateTime \"" << startTimeStr << "\" for \"" << desc
                << "\" scheduled period.  Format must be yyyy-mm-dd hh:mm:ss for one-time events"
                << " or hh:mm:ss for daily events." << endl;
            problemEncountered = true;
            continue;
        }
        time_t startTime = mktime(&tmTime);

        localtime_r(&now, &tmTime);
        strptimeRetVal = strptime(endTimeStr.c_str(), dateTimeFormat.c_str(), &tmTime);
        if (strptimeRetVal == nullptr) {
            cerr << "Unable to parse endDateTime \"" << endTimeStr << "\" for \"" << desc
                << "\" scheduled period.  Format must be yyyy-mm-dd hh:mm:ss for one-time events"
                << " or hh:mm:ss for daily events." << endl;
            problemEncountered = true;
            continue;
        }
        time_t endTime = mktime(&tmTime);

        SchedulePeriod newSchedulePeriod = {isDaily, desc, startTime, endTime};
        schedulePeriods.emplace_back(newSchedulePeriod);
    }

    return problemEncountered;
}


int ConfigReader::getWidgetPortNumberBase()
{
    if (!configObj["widgetPortNumberBase"].is_number()) {
        logMsg(LOG_ERR, "widgetPortNumberBase missing from configuration.");
        return 0;
    }
    return configObj["widgetPortNumberBase"].int_value();
}


bool ConfigReader::getWidgetGenerateSimulatedMeasurements(const std::string& widgetName)
{
    Json widgetConfig = getWidgetConfigJsonObject(widgetName);
    return widgetConfig["generateSimulatedMeasurements"].bool_value();
}


int ConfigReader::getWidgetAutoInactiveMs(const std::string& widgetName)
{
    Json widgetConfig = getWidgetConfigJsonObject(widgetName);
    // If autoInactiveMs isn't present, the value returned will
    // be zero, which disables the auto-inactive feature.
    return widgetConfig["autoInactiveMs"].int_value();
}

