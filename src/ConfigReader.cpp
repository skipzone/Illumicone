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

#include <fstream>
#include <iostream>
#include <sstream>

#include <libgen.h>
#include <unistd.h>

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
    bool problemEncountered = false;

    for (auto& periodConfigObj : jsonObj[scheduleName].array_items()) {

        string desc = periodConfigObj["description"].string_value();
        if (desc.empty()) {
            logger.logMsg(LOG_ERR, "Scheduled period has no description:  " + periodConfigObj.dump());
            problemEncountered = true;
            continue;
        }
        string startTimeStr = periodConfigObj["startDateTime"].string_value();
        if (startTimeStr.empty()) {
            logger.logMsg(LOG_ERR, "Scheduled period has no startDateTime:  " + periodConfigObj.dump());
            problemEncountered = true;
            continue;
        }
        string endTimeStr = periodConfigObj["endDateTime"].string_value();
        if (endTimeStr.empty()) {
            logger.logMsg(LOG_ERR, "Scheduled period has no endDateTime:  " + periodConfigObj.dump());
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
            logger.logMsg(LOG_ERR,
                          "Unable to parse startDateTime \"%s\" for \"%s\" scheduled period.  Format must"
                          " be yyyy-mm-dd hh:mm:ss for one-time events or hh:mm:ss for daily events.",
                          startTimeStr.c_str(), desc.c_str());
            problemEncountered = true;
            continue;
        }
        time_t startTime = mktime(&tmTime);

        localtime_r(&now, &tmTime);
        strptimeRetVal = strptime(endTimeStr.c_str(), dateTimeFormat.c_str(), &tmTime);
        if (strptimeRetVal == nullptr) {
            logger.logMsg(LOG_ERR,
                          "Unable to parse endDateTime \"%s\" for \"%s\" scheduled period.  Format must"
                          " be yyyy-mm-dd hh:mm:ss for one-time events or hh:mm:ss for daily events.",
                          endTimeStr.c_str(), desc.c_str());
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


bool ConfigReader::loadConfiguration(const std::string& configFilePathName)
{
    loadedConfigFilePathName = configFilePathName;

    // If the config file pathname is really a symbolic link, use the target
    // pathname because the path to any include files will be relative to the
    // target's location, not the symlink's location.
    char buf[512];
    int count = readlink(loadedConfigFilePathName.c_str(), buf, sizeof(buf));
    if (count >= 0 && (unsigned int) count < sizeof(buf)) {
        buf[count] = '\0';
        loadedConfigFilePathName = buf;
    }

    char* path = (char *) malloc(loadedConfigFilePathName.length() + 1);
    if (path == NULL) {
        logger.logMsg(LOG_ERR, "ConfigReader::loadConfiguration:  Can't allocate path buffer.");
        return false;
    }
    strcpy(path, loadedConfigFilePathName.c_str());
    loadedConfigFilePath = dirname(path);
    free(path);

    Json obj;
    if (!readConfigurationFile(loadedConfigFilePathName, obj)) {
        return false;
    }
    loadedConfigObj = resolveObjectIncludes(obj, 0);
    return !loadedConfigObj.is_null();
}


Json ConfigReader::resolveObjectIncludes(const Json& inputJsonObj, unsigned int curLevel)
{
    // TODO:  make sure curLevel is not too large (i.e., prevent recursive includes)

    Json::object objItems = inputJsonObj.object_items();    // this is really a map

    // Resolve all the _include_file_ elements at the top level of the object.
    for (auto it = objItems.begin(); it != objItems.end();) {
        if ((*it).first.find("_include_file_") == 0 && (*it).second.is_string())  {
            string includeFilePathName = loadedConfigFilePath + "/" + (*it).second.string_value();
            //logger.logMsg(LOG_DEBUG, "Reading " + (*it).first + " " + includeFilePathName
            //                + " at level " + to_string(curLevel) + ".");
            Json includeObj;
            if (!readConfigurationFile(includeFilePathName, includeObj)) {
                return Json();
            }
            objItems.insert(includeObj.object_items().begin(), includeObj.object_items().end());
            it = objItems.erase(it);
        }
        else {
            ++it;
        }
    }
    
    // Resolve all the _include_file_ elements in objects
    // and arrays of objects below the top level.
    for (auto&& kv : objItems) {
        if (kv.second.is_object()) {
            Json resolvedObj = resolveObjectIncludes(kv.second, curLevel + 1);
            if (resolvedObj.is_null()) {
                return Json();
            }
            objItems[kv.first] = resolvedObj;
        }
        else if (kv.second.is_array() && kv.second[0].is_object()) {
            Json::array arrayItems = kv.second.array_items();
            for (unsigned int i = 0; i < arrayItems.size(); ++i) {
                Json resolvedObj = resolveObjectIncludes(arrayItems[i], curLevel + 1);
                if (resolvedObj.is_null()) {
                    return Json();
                }
                arrayItems[i] = resolvedObj;
            }
            objItems[kv.first] = arrayItems;
        }
    }
    
    return Json(objItems);
}


bool ConfigReader::readConfigurationFile(const std::string& filePathName, Json& configObj)
{
    ifstream configFile(filePathName, ios_base::in);
    if (!configFile.is_open()) {
        logger.logMsg(LOG_ERR, "Can't open configuration file %s.", filePathName.c_str());
        return false;
    }
    stringstream jsonSstr;
    jsonSstr << configFile.rdbuf();
    configFile.close();

    string err;
    configObj = Json::parse(jsonSstr.str(), err, JsonParse::COMMENTS);
    if (!err.empty()) {
        logger.logMsg(LOG_ERR, "Parse of configuration file %s failed:  %s", filePathName.c_str(), err.c_str());
        return false;
    }

    return true;
}


string ConfigReader::dumpToString()
{
    return loadedConfigObj.dump();
}


string ConfigReader::getConfigFilePath()
{
    return loadedConfigFilePath;
}


string ConfigReader::getConfigFilePathName()
{
    return loadedConfigFilePathName;
}


Json ConfigReader::getConfigObject()
{
    return loadedConfigObj;
}


Json ConfigReader::getWidgetConfigJsonObject(const std::string& widgetName)
{
    for (auto& widgetConfigObj : loadedConfigObj["widgets"].array_items()) {
        if (widgetConfigObj["name"].string_value() == widgetName) {
            return widgetConfigObj;
        }
    }
    return Json("{}");
}


Json ConfigReader::getPatternConfigJsonObject(const string& patternName)
{
    for (auto& patternConfigObj : loadedConfigObj["patterns"].array_items()) {
        if (patternConfigObj["name"].string_value() == patternName) {
            return patternConfigObj;
        }
    }
    return Json("{}");
}

