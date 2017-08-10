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
#include "SpinnahWidget.h"
#include "WidgetId.h"

using namespace std;


SpinnahWidget::SpinnahWidget()
    : Widget(WidgetId::spinnah, 1)
{
    simulationUpdateIntervalMs[0] = 3000;
}


void SpinnahWidget::updateChannelSimulatedMeasurements(unsigned int chIdx)
{
    channels[chIdx]->getPosition();      // make sure previous velocity has been updated
    int newPosition = (channels[chIdx]->getPreviousPosition() + 1) % 65536;   // scale to 16-bit int from widget
    int newVelocity = newPosition % 51 * 10;    // limit to 500 rpm
    channels[chIdx]->setPositionAndVelocity(newPosition, newVelocity);
    channels[chIdx]->setIsActive(true);
}

