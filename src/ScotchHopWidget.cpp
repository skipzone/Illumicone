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
#include "ScotchHopWidget.h"

using namespace std;


extern Log logger;


ScotchHopWidget::ScotchHopWidget(WidgetId id)
    : Widget(id, 1, true)
{
    simulationUpdateIntervalMs[0] = 250;
    simulationSwitchId = 0;
    simulationTurningOn = true;
}


void ScotchHopWidget::updateChannelSimulatedMeasurements(unsigned int chIdx)
{
    logger.logMsg(LOG_DEBUG, channels[chIdx]->getName() + " simulationSwitchId=" + to_string(simulationSwitchId));
    channels[chIdx]->setPositionAndVelocity(simulationSwitchId, simulationTurningOn ? 1 : 0);
    channels[chIdx]->setIsActive(true);

    if (++simulationSwitchId >= simulationNumSwitches) {
        simulationSwitchId = 0;
        simulationTurningOn = !simulationTurningOn;
    }
}

