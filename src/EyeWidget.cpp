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
#include <string>
//#include <thread>
#include <time.h>
///#include <vector>

#include "ConfigReader.h"
#include "EyeWidget.h"
#include "illumiconeTypes.h"
#include "WidgetId.h"

using namespace std;


EyeWidget::EyeWidget()
    : Widget(WidgetId::eye, 1)
{
}


bool EyeWidget::moveData()
{
    if (!generateSimulatedMeasurements) {
        return true;
    }

    using namespace std::chrono;

    milliseconds epochMs = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
    unsigned int nowMs = epochMs.count();

    if (nowMs - lastUpdateMs > updateIntervalMs) {
        lastUpdateMs = nowMs;
        ++stepCount;
        if (channels[0]->getIsActive()) {
            if (stepCount < numActiveSteps) {
                channels[0]->setIsActive(true);     // so that it doesn't auto inactivate
                channels[0]->setPosition(activePositionValue);
                //cout << "position value for Eye set to " << activePositionValue << endl;
            }
            else {
                stepCount = 0;
                channels[0]->setIsActive(false);
                //cout << "Eye going inactive" << endl;
            }
        }
        else {
            if (stepCount < numInactiveSteps) {
                //cout << "Eye inactive" << endl;
            }
            else {
                stepCount = 0;
                channels[0]->setIsActive(true);
                channels[0]->setPosition(activePositionValue);
                //cout << "Eye going active; position value set to " << activePositionValue << endl;
            }
        }
    }

    return true;
}

