#include <chrono>
#include <iostream>
///#include <fstream>
///#include <regex>
#include <string>
//#include <thread>
#include <time.h>
///#include <vector>

#include "PlungerWidget.h"
#include "ConfigReader.h"
#include "illumiconeTypes.h"
#include "WidgetId.h"

using namespace std;


PlungerWidget::PlungerWidget()
    : Widget(WidgetId::plunger, 1)
{
    for (unsigned int i = 0; i < 8; ++i) {
        updateIntervalMs[i] = 0;
        lastUpdateMs[i] = 0;
    }

    updateIntervalMs[0] = 1000;
    updateIntervalMs[1] = 0;
    updateIntervalMs[2] = 0;
    updateIntervalMs[3] = 0;
    updateIntervalMs[4] = 0;
    updateIntervalMs[5] = 0;
    updateIntervalMs[6] = 0;
    updateIntervalMs[7] = 0;
}


bool PlungerWidget::moveData()
{
    if (!generateSimulatedMeasurements) {
        return true;
    }

    using namespace std::chrono;

    milliseconds epochMs = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
    unsigned int nowMs = epochMs.count();

    //cout << "---------- nowMs = " << nowMs << endl;

    for (unsigned int i = 0; i < numChannels; ++i) {
        //cout << "checking channel " << i << endl;
        if (updateIntervalMs[i] > 0 && nowMs - lastUpdateMs[i] > updateIntervalMs[i]) {
            //cout << "updating channel " << i << endl;
            lastUpdateMs[i] = nowMs;
            // TODO:  Widgets should not need to be aware of the cone dimensions (that's the pattern's job).
            //        For now, use a reasonable constant.  Eventually, replace this with simulation file playback.
            //channels[i]->setPositionAndVelocity((channels[i]->getPreviousPosition() + 1) % NUM_STRINGS, 0);
            channels[i]->setPositionAndVelocity((channels[i]->getPreviousPosition() + 1) % 36, 0);
            channels[i]->setIsActive(true);
            //cout << "updated channel " << i << endl;
        }
    }

    return true;
}

