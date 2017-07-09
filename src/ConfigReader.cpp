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

#include "ConfigReader.h"
#include <fstream>
#include "illumiconeTypes.h"
#include <iostream>
#include <sstream>

using namespace std;
using namespace json11;


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


Json ConfigReader::getWidgetConfigJsonObject(WidgetId widgetId)
{
    string widgetName = widgetIdToString(widgetId);
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
        if (patternConfigObj["patternName"].string_value() == patternName) {
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
    return configObj["patconIpAddress"].string_value();
}


bool ConfigReader::getSchedulePeriods(const std::string& scheduleName, std::vector<SchedulePeriod>& schedulePeriods)
{
    bool problemEncountered = false;

    for (auto& periodConfigObj : configObj[scheduleName].array_items()) {

        string desc = periodConfigObj["description"].string_value();
        if (desc.empty()) {
            cerr << "Shutoff period has no description:  " << periodConfigObj.dump() << endl;
            problemEncountered = true;
            continue;
        }
        string startTimeStr = periodConfigObj["startDateTime"].string_value();
        if (startTimeStr.empty()) {
            cerr << "Shutoff period has no startDateTime:  " << periodConfigObj.dump() << endl;
            problemEncountered = true;
            continue;
        }
        string endTimeStr = periodConfigObj["endDateTime"].string_value();
        if (endTimeStr.empty()) {
            cerr << "Shutoff period has no endDateTime:  " << periodConfigObj.dump() << endl;
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
                << "\" shutoff period.  Format must be yyyy-mm-dd hh:mm:ss for one-time events"
                << " or hh:mm:ss for daily events." << endl;
            problemEncountered = true;
            continue;
        }
        time_t startTime = mktime(&tmTime);

        localtime_r(&now, &tmTime);
        strptimeRetVal = strptime(endTimeStr.c_str(), dateTimeFormat.c_str(), &tmTime);
        if (strptimeRetVal == nullptr) {
            cerr << "Unable to parse endDateTime \"" << endTimeStr << "\" for \"" << desc
                << "\" shutoff period.  Format must be yyyy-mm-dd hh:mm:ss for one-time events"
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
    return configObj["widgetPortNumberBase"].int_value();
}


bool ConfigReader::getWidgetGenerateSimulatedMeasurements(WidgetId widgetId)
{
    Json widgetConfig = getWidgetConfigJsonObject(widgetId);
    return widgetConfig["generateSimulatedMeasurements"].bool_value();
}


int ConfigReader::getWidgetAutoInactiveMs(WidgetId widgetId)
{
    Json widgetConfig = getWidgetConfigJsonObject(widgetId);
    // If autoInactiveMs isn't present, the value returned will
    // be zero, which disables the auto-inactive feature.
    return widgetConfig["autoInactiveMs"].int_value();
}


/* =-=-=-=-=-=-=-=-=

    for (auto& autoShutoffPeriod : json["autoShutoffPeriods"].array_items()) {
        cout << "autoShutoffPeriod:  " << autoShutoffPeriod["description"].string_value() << endl;
        cout << "    startDate:  " << autoShutoffPeriod["startDate"].string_value() << endl;
        cout << "    startTime:  " << autoShutoffPeriod["startTime"].string_value() << endl;
        cout << "      endDate:  " << autoShutoffPeriod["endDate"].string_value() << endl;
        cout << "      endTime:  " << autoShutoffPeriod["endTime"].string_value() << endl;
    }

    for (auto& quiescentModePeriod : json["quiescentModePeriods"].array_items()) {
        cout << "quiescentModePeriod:  " << quiescentModePeriod["description"].string_value() << endl;
        cout << "      startDate:  " << quiescentModePeriod["startDate"].string_value() << endl;
        cout << "      startTime:  " << quiescentModePeriod["startTime"].string_value() << endl;
        cout << "        endDate:  " << quiescentModePeriod["endDate"].string_value() << endl;
        cout << "        endTime:  " << quiescentModePeriod["endTime"].string_value() << endl;
        cout << "    idlePattern:  " << quiescentModePeriod["idlePatternName"].string_value() << endl;
    }

    for (auto& widget : json["widgets"].array_items()) {
        cout << "widget:  " << widget.string_value() << endl;
    }

    for (auto& pattern : json["patterns"].array_items()) {
        cout << "pattern:  " << pattern["patternName"].string_value() << endl;
        for (auto& input : pattern["inputs"].array_items()) {
            cout << "    input:  " << input["inputName"].string_value() << endl;
            cout << "        widget:  " << input["widgetName"].string_value() << endl;
            cout << "        channel:  " << input["channelNumber"].int_value() << endl;
            cout << "        measurement:  " << input["measurement"].string_value() << endl;
        }
    }

*/


