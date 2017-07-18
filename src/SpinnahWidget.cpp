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

#include "illumiconeTypes.h"
#include "SpinnahWidget.h"
#include "WidgetId.h"

using namespace std;


SpinnahWidget::SpinnahWidget()
    : Widget(WidgetId::spinnah, 1)
{
    simulationUpdateIntervalMs[0] = 1000;
}


void SpinnahWidget::updateChannelSimulatedMeasurements(unsigned int chIdx)
{
    channels[chIdx]->setPositionAndVelocity(0, 120);
    channels[chIdx]->setIsActive(true);
}

