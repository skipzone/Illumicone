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

#include "illumiconeWidgetTypes.h"
#include "illumiconeUtility.h"
#include "Log.h"
#include "SpinnahWidget.h"
#include "WidgetId.h"

using namespace std;


extern Log logger;


SpinnahWidget::SpinnahWidget()
    : Widget(WidgetId::spinnah, 1)
{
    unsigned int nowMs = getNowMs();

    for (unsigned int i = 0; i < 8; ++i) {
        simulationToggleActivityPeriodMs[i] = 0;
        simulationToggleActivityMs[i] = nowMs;
        simulationIsActive[i] = true;
    }

    simulationUpdateIntervalMs[0] = 100;

    simulationToggleActivityPeriodMs[0] = 5000;
}


void SpinnahWidget::updateChannelSimulatedMeasurements(unsigned int chIdx)
{
    unsigned int nowMs = getNowMs();

    if (simulationToggleActivityPeriodMs[chIdx] != 0 && (int) (nowMs - simulationToggleActivityMs[chIdx]) >= 0) {
        simulationToggleActivityMs[chIdx] += simulationToggleActivityPeriodMs[chIdx];
        simulationIsActive[chIdx] = !simulationIsActive[chIdx];
    }

    if (simulationIsActive[chIdx]) {
        channels[chIdx]->getPosition();      // make sure previous velocity has been updated
        int newPosition = (channels[chIdx]->getPreviousPosition() + 1) % 65536;   // scale to 16-bit int from widget
        int newVelocity = newPosition % 51 * 10;    // limit to 500 rpm
        channels[chIdx]->setPositionAndVelocity(newPosition, newVelocity);
        channels[chIdx]->setIsActive(true);
        //logger.logMsg(LOG_DEBUG, "spinnah chIdx=" + to_string(chIdx)
        //                  + " newPosition=" + to_string(newPosition)
        //                  + " newVelocity=" + to_string(newVelocity));
    }
    else {
        channels[chIdx]->setIsActive(false);
    }
}

