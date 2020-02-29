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
#include "Log.h"
#include "FlowerWidget.h"

using namespace std;


extern Log logger;


FlowerWidget::FlowerWidget(WidgetId id)
    : Widget(id, 13)
{
    // Simulate yaw (channel 0).
    simulationUpdateIntervalMs[0] = 1;
    simulationMinValue[0] = 0;
    simulationMaxValue[0] = 3599;
    simulationStep[0] = 2;
    simulationUpDown[0] = false;
}


void FlowerWidget::updateChannelSimulatedMeasurements(unsigned int chIdx)
{
    // TODO:  maybe move all this stuff to Widget class as default behavior.

    // Make sure previous position and velocity have been
    // updated in case the pattern hasn't read them.
    channels[chIdx]->getPosition();

    int prevPosition = channels[chIdx]->getPreviousPosition();
    int newPosition;
    if (!simulatedPositionGoingDown) {
        newPosition = prevPosition + simulationStep[chIdx];
        if (newPosition > simulationMaxValue[chIdx]) {
            if (simulationUpDown[chIdx]) {
                newPosition = prevPosition;
                simulatedPositionGoingDown = true;
            }
            else {
                newPosition = simulationMinValue[chIdx];
            }
        }
    }
    else {
        newPosition = prevPosition - simulationStep[chIdx];
        if (newPosition < simulationMinValue[chIdx]) {
            newPosition = prevPosition;
            simulatedPositionGoingDown = false;
        }
    }

    //if (newPosition % 100 == 0) {
    //    logger.logMsg(LOG_DEBUG, channels[chIdx]->getName() + " newPosition=" + to_string(newPosition));
    //}

    channels[chIdx]->setPositionAndVelocity(newPosition, 0);
    channels[chIdx]->setIsActive(true);
}

