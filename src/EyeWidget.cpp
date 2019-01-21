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

//#include <string>

#include "EyeWidget.h"
#include "illumiconeWidgetTypes.h"
#include "Log.h"
#include "WidgetId.h"

using namespace std;


extern Log logger;


EyeWidget::EyeWidget()
    : Widget(WidgetId::eye, 1)
    , stepCount(0)
{
    simulationUpdateIntervalMs[0] = eyeSimulationUpdateIntervalMs;
}


void EyeWidget::updateChannelSimulatedMeasurements(unsigned int chIdx)
{
    ++stepCount;
    if (stepCount < numInactiveSteps) {
        if (makeAlwaysActive) {
            channels[chIdx]->setIsActive(true);     // so that it doesn't auto inactivate
            channels[chIdx]->setPosition(inactivePositionValue);
            //logger.logMsg(LOG_DEBUG, "position value for Eye set to inactive value " + to_string(inactivePositionValue));
        }
        else {
            channels[chIdx]->setIsActive(false);
            //logger.logMsg(LOG_DEBUG, "Eye inactive");
        }
    }
    else if (stepCount <= (numInactiveSteps + numActiveSteps)) {
        channels[chIdx]->setIsActive(true);     // so that it doesn't auto inactivate
        channels[chIdx]->setPosition(activePositionValue);
        //logger.logMsg(LOG_DEBUG, "position value for Eye set to active value " + to_string(activePositionValue));
    }
    else if (stepCount <= (numInactiveSteps + numActiveSteps + numSemiActiveSteps)) {
        channels[chIdx]->setIsActive(true);     // so that it doesn't auto inactivate
        channels[chIdx]->setPosition(semiActivePositionValue);
        //logger.logMsg(LOG_DEBUG, "position value for Eye set to semi-active value " + to_string(semiActivePositionValue));
    }
    else {
        stepCount = 0;
        //logger.logMsg(LOG_DEBUG, "Eye going inactive at next step");
    }
}

