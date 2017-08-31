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

#include "TriObeliskWidget.h"
#include "illumiconeUtility.h"
#include "illumiconeWidgetTypes.h"
#include "log.h"
#include "WidgetId.h"

using namespace std;


TriObeliskWidget::TriObeliskWidget()
    : Widget(WidgetId::triObelisk, 3)
{
    unsigned int nowMs = getNowMs();

    for (unsigned int i = 0; i < 8; ++i) {
        simulationToggleActivityPeriodMs[i] = 0;
        simulationToggleActivityMs[i] = nowMs;
        simulationIsActive[i] = true;
    }

    simulationUpdateIntervalMs[0] = 200;
    simulationUpdateIntervalMs[1] = 400;
    simulationUpdateIntervalMs[2] = 50;

    simulationToggleActivityPeriodMs[0] = 0;    //2500;
    simulationToggleActivityPeriodMs[0] = 0;    //5000;
    simulationToggleActivityPeriodMs[0] = 0;    //7500;

    minPosition[0] = -1000;
    minPosition[1] = -1000;
    minPosition[2] = -1000;

    maxPosition[0] = 1000;
    maxPosition[1] = 1000;
    maxPosition[2] = 1000;
}


void TriObeliskWidget::updateChannelSimulatedMeasurements(unsigned int chIdx)
{
    unsigned int nowMs = getNowMs();

    if (simulationToggleActivityPeriodMs[chIdx] != 0 && (int) (nowMs - simulationToggleActivityMs[chIdx]) >= 0) {
        simulationToggleActivityMs[chIdx] += simulationToggleActivityPeriodMs[chIdx];
        simulationIsActive[chIdx] = !simulationIsActive[chIdx];
    }

    if (simulationIsActive[chIdx]) {
        channels[chIdx]->getPosition();             // make sure previous velocity has been updated
        int newPosition = (channels[chIdx]->getPreviousPosition() + 1) % 65536;   // scale to 16-bit int from widget
        if (newPosition > maxPosition[chIdx]) {
            newPosition = minPosition[chIdx];
        }
        //logMsg(LOG_DEBUG, "chIdx=" + to_string(chIdx) + ", newPosition=" + to_string(newPosition));
        int newVelocity = newPosition % 51 * 10;    // limit to 500 rpm
        channels[chIdx]->setPositionAndVelocity(newPosition, newVelocity);
        channels[chIdx]->setIsActive(true);
    }
    else {
        channels[chIdx]->setIsActive(false);
    }
}

