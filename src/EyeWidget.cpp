#include <chrono>
#include <iostream>
#include <string>
//#include <thread>
#include <time.h>
///#include <vector>

#include "EyeWidget.h"
#include "illumiconeTypes.h"

using namespace std;


EyeWidget::EyeWidget()
    : Widget(WidgetId::eye, "Eye")
{
    for (unsigned int i = 0; i < 8; ++i) {
        updateIntervalMs[i] = 0;
        lastUpdateMs[i] = 0;
    }

    updateIntervalMs[0] = 200;
}


void EyeWidget::init(bool generateSimulatedMeasurements)
{
    this->generateSimulatedMeasurements = generateSimulatedMeasurements;

    channels.push_back(make_shared<WidgetChannel>(0, this));
}


bool EyeWidget::moveData()
{
    using namespace std::chrono;

    milliseconds epochMs = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
    unsigned int nowMs = epochMs.count();

    //cout << "---------- nowMs = " << nowMs << endl;

    for (unsigned int i = 0; i < getChannelCount(); ++i) {
        //cout << "checking channel " << i << endl;
        if (updateIntervalMs[i] > 0 && nowMs - lastUpdateMs[i] > updateIntervalMs[i]) {
            //cout << "updating channel " << i << endl;
            lastUpdateMs[i] = nowMs;
            channels[i]->setPositionAndVelocity((channels[i]->getPreviousPosition() + 1) % NUM_STRINGS, 0);
            channels[i]->setIsActive(true);
            //cout << "updated channel " << i << endl;
        }
    }

    return true;
}

