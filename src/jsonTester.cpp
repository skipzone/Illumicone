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

    std::ifstream t("test.json", ios_base::in);
    if (!t.is_open()) {
        cerr << "Can't open test.json." << endl;
        return 1;
    }
    std::stringstream jsonSstr;
    jsonSstr << t.rdbuf();
    t.close();

    string err;
    auto json = Json::parse(jsonSstr.str(), err, JsonParse::COMMENTS);
    if (!err.empty()) {
        cerr << "Parse failed:  " << err << endl;
        return 1;
    }

    cout << json.dump() << endl;

    return 0;
}