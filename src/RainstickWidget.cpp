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
#include <time.h>

#include "ConfigReader.h"
#include "illumiconeTypes.h"
#include "log.h"
#include "RainstickWidget.h"

using namespace std;


RainstickWidget::RainstickWidget()
    : Widget(WidgetId::rainstick, 7)
{
    for (unsigned int i = 0; i < 8; ++i) {
        updateIntervalMs[i] = 0;
        lastUpdateMs[i] = 0;
    }

    updateIntervalMs[2] = 10;
}


bool RainstickWidget::moveData()
{
    if (!generateSimulatedMeasurements) {
        return true;
    }

    using namespace std::chrono;

    milliseconds epochMs = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
    unsigned int nowMs = epochMs.count();

    for (unsigned int i = 0; i < numChannels; ++i) {
        if (updateIntervalMs[i] > 0 && (int) (nowMs - lastUpdateMs[i]) > updateIntervalMs[i]) {
            lastUpdateMs[i] = nowMs;
            channels[i]->getPosition();      // make sure previous position and velocity have been updated in case the pattern hasn't read them
            int newPosition = channels[i]->getPreviousPosition() + 1;
            //logMsg(LOG_DEBUG, "RainstickWidget::moveData:  newPosition=" + to_string(newPosition));
            if (newPosition > 1023) {
                newPosition = 0;
            }
            if (newPosition % 100 == 0) {
                logMsg(LOG_DEBUG, channels[i]->getName() + " newPosition=" + to_string(newPosition));
            }
            channels[i]->setPositionAndVelocity(newPosition, 0);
            channels[i]->setIsActive(true);
        }
    }

    return true;
}


