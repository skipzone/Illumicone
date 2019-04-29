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
#include <string>

#include "ConfigReader.h"
#include "json11.hpp"
#include "Log.h"

using namespace std;
using namespace json11;


Log logger;                     // this is the global Log object used everywhere


int main(int argc, char **argv) {

    if (argc != 2) {
        cout << "Usage:  " << argv[0] << " <configFileName>" << endl;
        return 2;
    }
    string jsonFileName(argv[1]);

    logger.startLogging("jsonTester", Log::LogTo::console);

    ConfigReader config;
    if (!config.readConfigurationFile(jsonFileName)) {
        return(1);
    }

    cout << config.dumpToString() << endl << endl;

/*
    if (!ConfigReader::getUnsignedIntValue(configObject, "numberOfStrings", numberOfStrings, errMsgSuffix)) return false;
    if (!ConfigReader::getUnsignedIntValue(configObject, "numberOfPixelsPerString", numberOfPixelsPerString, errMsgSuffix)) return false;

    cout << "numberOfStrings:  " << config.getNumberOfStrings() << endl;
    cout << "numberOfPixelsPerString:  " << config.getNumberOfPixelsPerString() << endl;
    cout << "opcServerIpAddress:  " << config.getOpcServerIpAddress() << endl;
    cout << "patconIpAddress:  " << config.getPatconIpAddress() << endl;
    cout << "widgetPortNumberBase:  " << config.getWidgetPortNumberBase() << endl;
*/

    Json configJsonObj = config.getConfigObject();

    Json patternControllerJsonObj;
    ConfigReader::getJsonObject(configJsonObj, "patternController", patternControllerJsonObj, "patternController");

    cout << endl << "---------- autoShutoffPeriods ----------" << endl;
    for (auto& shutoffPeriod : patternControllerJsonObj["shutoffPeriods"].array_items()) {
        cout << "  description:  " << shutoffPeriod["description"].string_value() << endl;
        cout << "startDateTime:  " << shutoffPeriod["startDateTime"].string_value() << endl;
        cout << "  endDateTime:  " << shutoffPeriod["endDateTime"].string_value() << endl;
        cout << "-----" << endl;
    }

    cout << endl << "---------- quiescentModePeriods ----------" << endl;
    for (auto& quiescentPeriod : patternControllerJsonObj["quiescentPeriods"].array_items()) {
        cout << "   description:  " << quiescentPeriod["description"].string_value() << endl;
        cout << " startDateTime:  " << quiescentPeriod["startDateTime"].string_value() << endl;
        cout << "   endDateTime:  " << quiescentPeriod["endDateTime"].string_value() << endl;
        cout << "quiescentColor:  " << quiescentPeriod["quiescentColor"].string_value() << endl;
        cout << "-----" << endl;
    }

    cout << endl << "---------- widgets ----------" << endl;
    for (auto& widget : patternControllerJsonObj["widgets"].array_items()) {
        cout << "widget:  " << widget["name"].string_value() << endl;
        cout << "    enabled:  " << widget["enabled"].bool_value() << endl;
        cout << "    generateSimulatedMeasurements:  " << widget["generateSimulatedMeasurements"].bool_value() << endl;
    }

    cout << endl << "---------- patterns ----------" << endl;
    for (auto& pattern : patternControllerJsonObj["patterns"].array_items()) {
        cout << "pattern:  " << pattern["name"].string_value() << endl;
        for (auto& input : pattern["inputs"].array_items()) {
            cout << "    input:  " << input["inputName"].string_value() << endl;
            cout << "        widget:  " << input["widgetName"].string_value() << endl;
            cout << "        channel:  " << input["channelNumber"].int_value() << endl;
            cout << "        measurement:  " << input["measurement"].string_value() << endl;
        }
    }

    logger.stopLogging();

    return 0;
}
