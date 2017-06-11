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
#include "json11.hpp"
#include <iostream>
#include <sstream>
#include <string>

using namespace std;
using namespace json11;


int main(int argc, char **argv) {

    if (argc != 2) {
        cout << "Usage:  jsonTester <fileName>" << endl;
        return 2;
    }
    string jsonFileName(argv[1]);

    ifstream t(jsonFileName, ios_base::in);
    if (!t.is_open()) {
        cerr << "Can't open " << jsonFileName << endl;
        return 1;
    }
    stringstream jsonSstr;
    jsonSstr << t.rdbuf();
    t.close();

    string err;
    auto json = Json::parse(jsonSstr.str(), err, JsonParse::COMMENTS);
    if (!err.empty()) {
        cerr << "Parse failed:  " << err << endl;
        return 1;
    }

    cout << json.dump() << endl << endl;

    cout << "numberOfStrings:  " << json["numberOfStrings"].int_value() << endl;
    cout << "numberOfPixelsPerString:  " << json["numberOfPixelsPerString"].int_value() << endl;
    cout << "opcServerIpAddress:  " << json["opcServerIpAddress"].string_value() << endl;
    cout << "patconIpAddress:  " << json["patconIpAddress"].string_value() << endl;
    cout << "widgetPortNumberBase:  " << json["widgetPortNumberBase"].int_value() << endl;

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

    return 0;
}
