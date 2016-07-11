#include <chrono>
#include <iostream>
///#include <fstream>
///#include <regex>
#include <string>
//#include <thread>
#include <time.h>
///#include <vector>

#include "ThreeWheelWidget.h"
#include "illumiconeTypes.h"
#include "WidgetId.h"

using namespace std;


ThreeWheelWidget::ThreeWheelWidget()
    : Widget(WidgetId::triObelisk, "TriObelisk")
{
    for (unsigned int i = 0; i < 8; ++i) {
        updateIntervalMs[i] = 0;
        lastUpdateMs[i] = 0;
    }

    updateIntervalMs[0] = 50;
    updateIntervalMs[1] = 100;
    updateIntervalMs[2] = 150;
}


void ThreeWheelWidget::init(bool generateSimulatedMeasurements)
{
    this->generateSimulatedMeasurements = generateSimulatedMeasurements;

    for (int i = 0; i < 3; ++i) {
        channels.push_back(make_shared<WidgetChannel>(i, this));
    }

    if (!generateSimulatedMeasurements) {
        startUdpRxThread();
    }
}


bool ThreeWheelWidget::moveData()
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
            //cout << "updating channel " << i << endl;
            lastUpdateMs[i] = nowMs;
            channels[i]->setPositionAndVelocity((channels[i]->getPreviousPosition() + 1) % NUM_STRINGS, 0);
            channels[i]->setIsActive(true);
            //cout << "updated channel " << i << endl;
        }
    }

    return true;
}

