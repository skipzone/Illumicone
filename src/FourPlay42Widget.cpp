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
///#include <fstream>
///#include <regex>
#include <string>
//#include <thread>
#include <time.h>
///#include <vector>

#include "FourPlay42Widget.h"
#include "ConfigReader.h"
#include "illumiconeTypes.h"
#include "WidgetId.h"

using namespace std;


FourPlay42Widget::FourPlay42Widget()
    : Widget(WidgetId::fourPlay42, 4)
{
    for (unsigned int i = 0; i < 8; ++i) {
        updateIntervalMs[i] = 0;
        lastUpdateMs[i] = 0;
    }

    updateIntervalMs[0] = 100;
    updateIntervalMs[1] = 200;
    updateIntervalMs[2] = 400;
    updateIntervalMs[3] = 50;
    updateIntervalMs[4] = 0;
    updateIntervalMs[5] = 0;
    updateIntervalMs[6] = 0;
    updateIntervalMs[7] = 0;
}


bool FourPlay42Widget::moveData()
{
    if (!generateSimulatedMeasurements) {
        return true;
    }

    using namespace std::chrono;

    milliseconds epochMs = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
    unsigned int nowMs = epochMs.count();

    //cout << "---------- nowMs = " << nowMs << endl;

    for (unsigned int i = 0; i < numChannels; ++i) {
        if (updateIntervalMs[i] > 0 && nowMs - lastUpdateMs[i] > updateIntervalMs[i]) {
            lastUpdateMs[i] = nowMs;
            channels[i]->getPosition();      // make sure previous velocity has been updated
            int newPosition = (channels[i]->getPreviousPosition() + 1) % 65536;   // scale to 16-bit int from widget
            int newVelocity = newPosition % 51 * 10;    // limit to 500 rpm
            //cout << "newPosition=" << newPosition << ", newVelocity=" << newVelocity << endl;
            channels[i]->setPositionAndVelocity(newPosition, newVelocity);
            channels[i]->setIsActive(true);
        }
    }

    return true;
}

