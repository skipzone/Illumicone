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

#include "BellsWidget.h"
#include "ConfigReader.h"
#include "illumiconeWidgetTypes.h"
#include "WidgetId.h"

using namespace std;


BellsWidget::BellsWidget()
    : Widget(WidgetId::bells, 1)
    , simWidth(0)
{
    simulationUpdateIntervalMs[0] = 1000;
    simulationUpdateIntervalMs[1] = 1000;
    simulationUpdateIntervalMs[2] = 1000;
}


void BellsWidget::updateChannelSimulatedMeasurements(unsigned int chIdx)
{
    if (simWidth == 0) {
        // TODO:  Widgets should not need to be aware of the cone dimensions (that's the pattern's job).
        //        For now, use a reasonable constant.  Eventually, replace this with simulation file playback.
        //simWidth = NUM_STRINGS / 3;
        simWidth = 36 / 3;
    }

    channels[chIdx]->setPositionAndVelocity(simWidth, 0);
    simWidth--;

    channels[chIdx]->setIsActive(true);
}

