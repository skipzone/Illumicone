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


#include <assert.h>
#include <iomanip>
#include <iostream>

#include <unistd.h>

#include "Log.h"

using namespace std;


Log logger;                     // this is the global Log object used everywhere


int main(int argc, char **argv)
{
    cout << "Illumicone logging test." << endl;

    logger.startLogging("logTest", Log::LogTo::file);

    logger.setAutoLogRotation(1, 0, 0);

    for (int i = 0; i < 200; ++i) {
        logger.logMsg(LOG_INFO, "i=%d", i);
        sleep(1);
    }

    logger.stopLogging();

    cout << "Done." << endl;
}

