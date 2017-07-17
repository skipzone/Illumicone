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

#include <chrono>
#include <iostream>
#include <iomanip>
#include <sstream>
//#include <stdlib.h>
//#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include "log.h"


using namespace std;


const string getTimestamp()
{
    using namespace std::chrono;

    milliseconds epochMs = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
    int ms = epochMs.count() % 1000;
    time_t now = epochMs.count() / 1000;

    struct tm tmStruct = *localtime(&now);
    char buf[20];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tmStruct);

    stringstream sstr;
    sstr << buf << "." << setfill('0') << setw(3) << ms << ":  ";

    string str = sstr.str();
    return str;
}


void logMsg(int priority, const string& message)
{
  if (priority <= LOG_WARNING) {
    cout << getTimestamp() << message << endl;
  }
  else {
    cerr << getTimestamp() << message << endl;
  }
}


