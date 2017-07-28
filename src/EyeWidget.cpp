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

#include "EyeWidget.h"
#include "illumiconeWidgetTypes.h"
#include "log.h"
#include "WidgetId.h"

using namespace std;


EyeWidget::EyeWidget()
    : Widget(WidgetId::eye, 1)
{
    simulationUpdateIntervalMs[0] = eyeSimulationUpdateIntervalMs;
}


void EyeWidget::updateChannelSimulatedMeasurements(unsigned int chIdx)
{
    ++stepCount;
    if (channels[chIdx]->getIsActive()) {
        if (stepCount < numActiveSteps) {
            channels[chIdx]->setIsActive(true);     // so that it doesn't auto inactivate
            channels[chIdx]->setPosition(activePositionValue);
            //logMsg(LOG_DEBUG, "position value for Eye set to " + to_string(activePositionValue));
        }
        else {
            stepCount = 0;
            channels[chIdx]->setIsActive(false);
            //logMsg(LOG_DEBUG, "Eye going inactive");
        }
    }
    else {
        if (stepCount < numInactiveSteps) {
            //logMsg(LOG_DEBUG, "Eye inactive");
        }
        else {
            stepCount = 0;
            channels[chIdx]->setIsActive(true);
            channels[chIdx]->setPosition(activePositionValue);
            //logMsg(LOG_DEBUG, "Eye going active; position value set to " + to_string(activePositionValue));
        }
    }
}

