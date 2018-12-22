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
#include "BatonWidget.h"

using namespace std;


BatonWidget::BatonWidget()
    : Widget(WidgetId::baton, 13)
{
    // Simulate acceleration.
    simulationUpdateIntervalMs[6] = 10;
    simulationUpdateIntervalMs[7] = 15;
    simulationUpdateIntervalMs[8] = 20;
}


void BatonWidget::updateChannelSimulatedMeasurements(unsigned int chIdx)
{
    // Make sure previous position and velocity have been
    // updated in case the pattern hasn't read them.
    channels[chIdx]->getPosition();

    int newPosition = channels[chIdx]->getPreviousPosition();
    if (!simulatedPositionGoingDown) {
        if (newPosition < 1023) {
            ++newPosition;
        }
        else {
            simulatedPositionGoingDown = true;
        }
    }
    else {
        if (newPosition > 0) {
            --newPosition;
        }
        else {
            simulatedPositionGoingDown = false;
        }
    }

    //if (newPosition % 100 == 0) {
    //    logMsg(LOG_DEBUG, channels[chIdx]->getName() + " newPosition=" + to_string(newPosition));
    //}

    channels[chIdx]->setPositionAndVelocity(newPosition, 0);
    channels[chIdx]->setIsActive(true);
}

