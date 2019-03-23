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

#include <algorithm>
#include <climits>
#include <string>

#include "BellsWidget.h"
#include "ConfigReader.h"
#include "illumiconeWidgetTypes.h"
#include "Log.h"
#include "WidgetId.h"

using namespace std;


extern Log logger;


BellsWidget::BellsWidget()
    : Widget(WidgetId::bells, 3)
    , simStep(INT_MIN + stepSize)
{
    simulationUpdateIntervalMs[0] = 20;
    simulationUpdateIntervalMs[1] = 20;
    simulationUpdateIntervalMs[2] = 20;
}


void BellsWidget::updateChannelSimulatedMeasurements(unsigned int chIdx)
{
    simStep -= stepSize;
    if (simStep <= doStrikeAtStepNum) {
        simStep = stepNumAtStrike;
    }
    if (simStep >= 0) {
        channels[chIdx]->setPositionAndVelocity(simStep, 0);
        channels[chIdx]->setIsActive(true);
        //logger.logMsg(LOG_DEBUG, channels[chIdx]->getName() + " position=" + to_string(simStep));
    }
    else {
        channels[chIdx]->setPositionAndVelocity(0, 0);
        channels[chIdx]->setIsActive(false);
    }
}

