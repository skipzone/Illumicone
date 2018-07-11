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

#include <string>

#include "ConfigReader.h"
#include "illumiconeWidgetTypes.h"
#include "illumiconeUtility.h"
#include "log.h"
#include "BoogieBoardWidget.h"

using namespace std;


BoogieBoardWidget::BoogieBoardWidget()
    : Widget(WidgetId::boogieBoard, 6)
{
    // yaw, pitch, roll, which vary from -9000 to 9000
    simulationUpdateIntervalMs[0] = 50;
    simulationUpdateIntervalMs[1] = 50;
    simulationUpdateIntervalMs[2] = 50;
    // x, y, and z acceleration, which vary from -40 to 40
    simulationUpdateIntervalMs[3] = 200;
    simulationUpdateIntervalMs[4] = 200;
    simulationUpdateIntervalMs[5] = 200;
}


void BoogieBoardWidget::updateChannelSimulatedMeasurements(unsigned int chIdx)
{
    int minPos;
    int maxPos;
    int step;
    int logInterval;
    switch (chIdx) {
        case 0:
        case 1:
        case 2:
            minPos = -9000;
            maxPos = 9000;
            step = 20;
            logInterval = 100;
            break;
        case 3:
        case 4:
        case 5:
            minPos = -40;
            maxPos = 40;
            step = 1;
            logInterval = 5;
            break;
    }

    // Make sure previous position and velocity have been
    // updated in case the pattern hasn't read them.
    channels[chIdx]->getPosition();

    int newPosition = channels[chIdx]->getPreviousPosition();
    if (!simulatedPositionGoingDown) {
        if (newPosition < maxPos) {
            newPosition += step;
        }
        else {
            simulatedPositionGoingDown = true;
        }
    }
    else {
        if (newPosition > minPos) {
            newPosition -= step;
        }
        else {
            simulatedPositionGoingDown = false;
        }
    }

    if (newPosition % logInterval == 0) {
        logMsg(LOG_DEBUG, channels[chIdx]->getName() + " newPosition=" + to_string(newPosition));
    }

    channels[chIdx]->setPositionAndVelocity(newPosition, 0);
    channels[chIdx]->setIsActive(true);
}

