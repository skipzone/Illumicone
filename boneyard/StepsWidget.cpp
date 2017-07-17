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

#include <iostream>
#include <vector>
#include <chrono>

#include <string>
#include <time.h>

#include "StepsWidget.h"
#include "illumiconeTypes.h"

using namespace std;


StepsWidget::StepsWidget()
    : Widget(WidgetId::steps)
{
    for (unsigned int i = 0; i < 8; ++i) {
        updateIntervalMs[i] = 0;
        lastUpdateMs[i] = 0;
    }

    updateIntervalMs[0] = 3000;
    updateIntervalMs[1] = 3000;
    updateIntervalMs[2] = 3000;
    updateIntervalMs[3] = 3000;
    updateIntervalMs[4] = 3000;
    updateIntervalMs[5] = 0;
    updateIntervalMs[6] = 0;
    updateIntervalMs[7] = 0;
}


void StepsWidget::init(bool generateSimulatedMeasurements)
{
    this->generateSimulatedMeasurements = generateSimulatedMeasurements;

    for (int i = 0; i < 5; ++i) {
        channels.push_back(make_shared<WidgetChannel>(i, this));
    }

    if (!generateSimulatedMeasurements) {
        startUdpRxThread();
    }
}


bool StepsWidget::moveData()
{
    if (!generateSimulatedMeasurements) {
        return true;
    }

    using namespace std::chrono;

    milliseconds epochMs = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
    unsigned int nowMs = epochMs.count();

    //cout << "---------- nowMs = " << nowMs << endl;

    for (unsigned int i = 0; i < getChannelCount(); ++i) {
//        cout << "checking channel " << i << endl;
        if (updateIntervalMs[i] > 0 && (nowMs - lastUpdateMs[i] > updateIntervalMs[i])) {
//            cout << "updating channel " << i << endl;
            lastUpdateMs[i] = nowMs;
            channels[i]->setPositionAndVelocity((channels[i]->getPreviousPosition() + 1) % 3, 0);
            channels[i]->setIsActive(true);
//            cout << "updated channel " << i << endl;
        }
    }


    return true;
}
