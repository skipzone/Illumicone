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

#include <chrono>
#include <iostream>
#include <string>
//#include <thread>
#include <time.h>
///#include <vector>

#include "BellsWidget.h"
#include "illumiconeTypes.h"
#include "WidgetId.h"

using namespace std;

static int simWidth;

BellsWidget::BellsWidget()
    : Widget(WidgetId::bells, "Bells")
{
    for (unsigned int i = 0; i < 8; ++i) {
        updateIntervalMs[i] = 0;
        lastUpdateMs[i] = 0;
    }

    updateIntervalMs[0] = 1000;
    updateIntervalMs[1] = 1000;
    updateIntervalMs[2] = 1000;
    updateIntervalMs[3] = 0;
    updateIntervalMs[4] = 0;
    updateIntervalMs[5] = 0;
    updateIntervalMs[6] = 0;
    updateIntervalMs[7] = 0;

    simWidth = 0;
}


void BellsWidget::init(bool generateSimulatedMeasurements)
{
    this->generateSimulatedMeasurements = generateSimulatedMeasurements;

    channels.push_back(make_shared<WidgetChannel>(0, this));

    if (!generateSimulatedMeasurements) {
        startUdpRxThread();
    }
}


bool BellsWidget::moveData()
{
    if (!generateSimulatedMeasurements) {
        return true;
    }

    using namespace std::chrono;

    milliseconds epochMs = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
    unsigned int nowMs = epochMs.count();

    //cout << "---------- nowMs = " << nowMs << endl;

    for (unsigned int i = 0; i < getChannelCount(); ++i) {
        //cout << "checking channel " << i << endl;
        if (updateIntervalMs[i] > 0 && nowMs - lastUpdateMs[i] > updateIntervalMs[i]) {
            int prevPos = channels[i]->getPreviousPosition();
            //cout << "updating channel " << i << endl;
            lastUpdateMs[i] = nowMs;

            if (simWidth == 0) {
                simWidth = NUM_STRINGS / 3;
            }

            channels[i]->setPositionAndVelocity(simWidth, 0);
            simWidth--;

            channels[i]->setIsActive(true);
            //cout << "updated channel " << i << endl;
        }
    }

    return true;
}
