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
#include "json11.hpp"
#include <iostream>
#include <sstream>
#include <string>

using namespace std;
using namespace json11;


int main(int argc, char **argv) {

    if (argc != 2) {
        cout << "Usage:  " << argv[0] << " <configFileName>" << endl;
        return 2;
    }
    string jsonFileName(argv[1]);

    ConfigReader config;
    if (!config.readConfigurationFile(jsonFileName)) {
        return(1);
    }

    cout << config.dumpToString() << endl << endl;

    cout << "numberOfStrings:  " << config.getNumberOfStrings() << endl;
    cout << "numberOfPixelsPerString:  " << config.getNumberOfPixelsPerString() << endl;
    cout << "opcServerIpAddress:  " << config.getOpcServerIpAddress() << endl;
    cout << "patconIpAddress:  " << config.getPatconIpAddress() << endl;
    cout << "widgetPortNumberBase:  " << config.getWidgetPortNumberBase() << endl;

    Json configJsonObj = config.getJsonObject();

    for (auto& autoShutoffPeriod : configJsonObj["autoShutoffPeriods"].array_items()) {
        cout << "autoShutoffPeriod:  " << autoShutoffPeriod["description"].string_value() << endl;
        cout << "    startDate:  " << autoShutoffPeriod["startDate"].string_value() << endl;
        cout << "    startTime:  " << autoShutoffPeriod["startTime"].string_value() << endl;
        cout << "      endDate:  " << autoShutoffPeriod["endDate"].string_value() << endl;
        cout << "      endTime:  " << autoShutoffPeriod["endTime"].string_value() << endl;
    }

    for (auto& quiescentModePeriod : configJsonObj["quiescentModePeriods"].array_items()) {
        cout << "quiescentModePeriod:  " << quiescentModePeriod["description"].string_value() << endl;
        cout << "      startDate:  " << quiescentModePeriod["startDate"].string_value() << endl;
        cout << "      startTime:  " << quiescentModePeriod["startTime"].string_value() << endl;
        cout << "        endDate:  " << quiescentModePeriod["endDate"].string_value() << endl;
        cout << "        endTime:  " << quiescentModePeriod["endTime"].string_value() << endl;
        cout << "    idlePattern:  " << quiescentModePeriod["idlePatternName"].string_value() << endl;
    }

    for (auto& widget : configJsonObj["widgets"].array_items()) {
        cout << "widget:  " << widget["name"].string_value() << endl;
        cout << "    enabled:  " << widget["enabled"].bool_value() << endl;
        cout << "    generateSimulatedMeasurements:  " << widget["generateSimulatedMeasurements"].bool_value() << endl;
    }

    for (auto& pattern : configJsonObj["patterns"].array_items()) {
        cout << "pattern:  " << pattern["patternName"].string_value() << endl;
        for (auto& input : pattern["inputs"].array_items()) {
            cout << "    input:  " << input["inputName"].string_value() << endl;
            cout << "        widget:  " << input["widgetName"].string_value() << endl;
            cout << "        channel:  " << input["channelNumber"].int_value() << endl;
            cout << "        measurement:  " << input["measurement"].string_value() << endl;
        }
    }

    return 0;
}
