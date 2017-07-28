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
#include "log.h"
#include "PumpWidget.h"
#include "WidgetId.h"

using namespace std;


PumpWidget::PumpWidget()
    : Widget(WidgetId::pump, 1)
{
    simulationUpdateIntervalMs[0] = 10;
}


void PumpWidget::updateChannelSimulatedMeasurements(unsigned int chIdx)
{
    int newPosition = channels[chIdx]->getPreviousPosition() + 1;
    if (newPosition > 1023) {
        newPosition = 0;
    }
    //if (newPosition % 10 == 0) {
    //    logMsg(LOG_DEBUG, channels[chIdx]->getName() + " newPosition=" + to_string(newPosition));
    //}
    channels[chIdx]->setPositionAndVelocity(newPosition, 0);
    channels[chIdx]->setIsActive(true);
}

