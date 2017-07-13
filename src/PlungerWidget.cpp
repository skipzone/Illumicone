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

    updateIntervalMs[0] = 100;
}


bool PlungerWidget::moveData()
{
    if (!generateSimulatedMeasurements) {
        return true;
    }

    using namespace std::chrono;

    milliseconds epochMs = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
    unsigned int nowMs = epochMs.count();

    for (unsigned int i = 0; i < numChannels; ++i) {
        if (updateIntervalMs[i] > 0 && nowMs - lastUpdateMs[i] > updateIntervalMs[i]) {
            lastUpdateMs[i] = nowMs;
            int newPosition = channels[i]->getPreviousPosition() + 1;
            if (newPosition > 1023) {
                newPosition = 0;
            }
            channels[i]->setPositionAndVelocity(newPosition, 0);
            channels[i]->setIsActive(true);
        }
    }

    return true;
}

