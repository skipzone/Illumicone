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

// TODO 7/31/2017 ross:  Use logger.logMsg in place of cerr.

#include <fstream>
#include <iostream>
#include <sstream>

#include "ConfigReader.h"
#include "illumiconePixelUtility.h"
#include "Log.h"

using namespace std;
using namespace json11;


extern Log logger;


bool ConfigReader::getBoolValue(const json11::Json& jsonObj,
                                const std::string& name,
                                bool& value,
                                const std::string& errorMessageSuffix)
{
    if (!jsonObj[name].is_bool()) {
        if (!errorMessageSuffix.empty()) {
            logger.logMsg(LOG_ERR, name + " is not present or is not a boolean value" + errorMessageSuffix);
        }
        return false;
    }
    value = jsonObj[name].bool_value();
    return true;
}


bool ConfigReader::getDoubleValue(const json11::Json& jsonObj,
                                  const std::string& name,
                                  double& value,
                                  const std::string& errorMessageSuffix,
                                  double minValue,
                                  double maxValue)
{
    if (!jsonObj[name].is_number()) {
        if (!errorMessageSuffix.empty()) {
            logger.logMsg(LOG_ERR, name + " is not present or is not a number" + errorMessageSuffix);
        }
        return false;
    }
    value = jsonObj[name].number_value();
    if (value < minValue || value > maxValue) {
        if (!errorMessageSuffix.empty()) {
            logger.logMsg(LOG_ERR, name + " is outside of range [" + to_string(minValue)
                            + ", " + to_string(maxValue) + "]" + errorMessageSuffix);
        }
        return false;
    }
    return true;
}


bool ConfigReader::getFloatValue(const json11::Json& jsonObj,
                                 const std::string& name,
                                 float& value,
                                 const std::string& errorMessageSuffix,
                                 float minValue,
                                 float maxValue)
{
    if (!jsonObj[name].is_number()) {
        if (!errorMessageSuffix.empty()) {
            logger.logMsg(LOG_ERR, name + " is not present or is not a number" + errorMessageSuffix);
        }
        return false;
    }
    value = jsonObj[name].number_value();
    if (value < minValue || value > maxValue) {
        if (!errorMessageSuffix.empty()) {
            logger.logMsg(LOG_ERR, name + " is outside of range [" + to_string(minValue)
                            + ", " + to_string(maxValue) + "]" + errorMessageSuffix);
        }
        return false;
    }
    return true;
}


bool ConfigReader::getHsvPixelValue(const json11::Json& jsonObj,
                                    const std::string& name,
                                    std::string& rgbStr,
                                    HsvPixel& value,
                                    const std::string& errorMessageSuffix,
                                    bool allowEmptyString,
                                    const HsvPixel& defaultValue)
{
    if (!jsonObj[name].is_string()) {
        if (!errorMessageSuffix.empty()) {
            logger.logMsg(LOG_ERR, name + " is not present or is not a string" + errorMessageSuffix);
        }
        return false;
    }
    rgbStr = jsonObj[name].string_value();
    if (rgbStr.empty()) {
        if (!allowEmptyString) {
            if (!errorMessageSuffix.empty()) {
                logger.logMsg(LOG_ERR, name + " is empty" + errorMessageSuffix);
            }
            return false;
        }
        value = defaultValue;
        return true;
    }
    if (!stringToHsvPixel(rgbStr, value)) {
        if (!errorMessageSuffix.empty()) {
            logger.logMsg(LOG_ERR, name + " is not a valid HSV color" + errorMessageSuffix);
        }
        return false;
    }
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
        if (!errorMessageSuffix.empty()) {
            logger.logMsg(LOG_ERR, name + " is not present or is not a number" + errorMessageSuffix);
        }
        return false;
    }
    value = jsonObj[name].int_value();
    if (value < minValue || value > maxValue) {
        if (!errorMessageSuffix.empty()) {
            logger.logMsg(LOG_ERR, name + " is outside of range [" + to_string(minValue)
                            + ", " + to_string(maxValue) + "]" + errorMessageSuffix);
        }
        return false;
    }
    return true;
}


bool ConfigReader::getJsonObject(const json11::Json& jsonObj,
                                 const std::string& name,
                                 json11::Json& value,
                                 const std::string& errorMessageSuffix)
{
    if (!jsonObj[name].is_object()) {
        if (!errorMessageSuffix.empty()) {
            logger.logMsg(LOG_ERR, name + " is not present or is not a Json object" + errorMessageSuffix);
        }
        return false;
    }
    value = jsonObj[name];
    return true;
}


bool ConfigReader::getRgbPixelValue(const json11::Json& jsonObj,
                                    const std::string& name,
                                    std::string& rgbStr,
                                    RgbPixel& value,
                                    const std::string& errorMessageSuffix,
                                    bool allowEmptyString,
                                    const RgbPixel& defaultValue)
{
    if (!jsonObj[name].is_string()) {
        if (!errorMessageSuffix.empty()) {
            logger.logMsg(LOG_ERR, name + " is not present or is not a string" + errorMessageSuffix);
        }
        return false;
    }
    rgbStr = jsonObj[name].string_value();
    if (rgbStr.empty()) {
        if (!allowEmptyString) {
            if (!errorMessageSuffix.empty()) {
                logger.logMsg(LOG_ERR, name + " is empty" + errorMessageSuffix);
            }
            return false;
        }
        value = defaultValue;
        return true;
    }
    if (!stringToRgbPixel(rgbStr, value)) {
        if (!errorMessageSuffix.empty()) {
            logger.logMsg(LOG_ERR, name + " is not a valid RGB color" + errorMessageSuffix);
        }
        return false;
    }
    return true;
}


bool ConfigReader::getSchedulePeriods(const json11::Json& jsonObj,
                                      const std::string& scheduleName,
                                      std::vector<SchedulePeriod>& schedulePeriods)
{
    // TODO:  modify to use logger rather than cerr and to use errorMessageSuffix.

    bool problemEncountered = false;

    for (auto& periodConfigObj : jsonObj[scheduleName].array_items()) {

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

        SchedulePeriod newSchedulePeriod = {isDaily, desc, startTime, endTime, periodConfigObj};
        schedulePeriods.emplace_back(newSchedulePeriod);
    }

    return !problemEncountered;
}


bool ConfigReader::getStringValue(const json11::Json& jsonObj,
                                  const std::string& name,
                                  string& value,
                                  const std::string& errorMessageSuffix,
                                  bool allowEmptyString)
{
    if (!jsonObj[name].is_string()) {
        if (!errorMessageSuffix.empty()) {
            logger.logMsg(LOG_ERR, name + " is not present or is not a string" + errorMessageSuffix);
        }
        return false;
    }
    value = jsonObj[name].string_value();
    if (!allowEmptyString && value.empty()) {
        if (!errorMessageSuffix.empty()) {
            logger.logMsg(LOG_ERR, name + " is empty" + errorMessageSuffix);
        }
        return false;
    }
    return true;
}


bool ConfigReader::getUnsignedIntValue(const json11::Json& jsonObj,
                                       const std::string& name,
                                       unsigned int& value,
                                       const std::string& errorMessageSuffix,
                                       unsigned int minValue,
                                       unsigned int maxValue)
{
    if (!jsonObj[name].is_number()) {
        if (!errorMessageSuffix.empty()) {
            logger.logMsg(LOG_ERR, name + " is not present or is not a number" + errorMessageSuffix);
        }
        return false;
    }
    value = jsonObj[name].int_value();
    if (value < minValue || value > maxValue) {
        if (!errorMessageSuffix.empty()) {
            logger.logMsg(LOG_ERR, name + " is outside of range [" + to_string(minValue)
                            + ", " + to_string(maxValue) + "]" + errorMessageSuffix);
        }
        return false;
    }
    return true;
}


json11::Json ConfigReader::mergeConfigObjects(const json11::Json& primaryJsonObj, const json11::Json& secondaryJsonObj)
{
    Json::object merged = primaryJsonObj.object_items();
    merged.insert(secondaryJsonObj.object_items().begin(), secondaryJsonObj.object_items().end()); 
    return Json(merged);
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


Json ConfigReader::getConfigObject()
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


bool ConfigReader::getUseTcpForOpcServer()
{
    bool val;
    return getBoolValue(configObj, "useTcpForOpcServer", val, ".") ? val : false;
}


string ConfigReader::getOpcServerIpAddress()
{
    string val;
    return getStringValue(configObj, "opcServerIpAddress", val, ".") ? val : "";
}


unsigned int ConfigReader::getOpcServerPortNumber()
{
    unsigned int val;
    return getUnsignedIntValue(configObj, "opcServerPortNumber", val, ".", 1024, 65535) ? val : 0;
}


string ConfigReader::getPatconIpAddress()
{
    string val;
    return getStringValue(configObj, "patconIpAddress", val, ".") ? val : "";
}


unsigned int ConfigReader::getRadioPollingLoopSleepIntervalUs()
{
    unsigned int val;
    return getUnsignedIntValue(configObj, "radioPollingLoopSleepIntervalUs", val, ".", 1) ? val : 0;
}


int ConfigReader::getWidgetPortNumberBase()
{
    unsigned int val;
    return getUnsignedIntValue(configObj, "widgetPortNumberBase", val, ".", 1024, 65535) ? val : 0;
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

