#include <chrono>
#include <iostream>
#include <string>
#include <time.h>

#include "ConfigReader.h"
#include "illumiconeTypes.h"
#include "log.h"
#include "PumpWidget.h"
#include "WidgetId.h"

using namespace std;


PumpWidget::PumpWidget()
    : Widget(WidgetId::pump, 1)
{
    for (unsigned int i = 0; i < 8; ++i) {
        updateIntervalMs[i] = 0;
        lastUpdateMs[i] = 0;
    }

    updateIntervalMs[0] = 10;
}


bool PumpWidget::moveData()
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
            //if (newPosition % 10 == 0) {
            //    logMsg(LOG_DEBUG, channels[i]->getName() + " newPosition=" + to_string(newPosition));
            //}
            channels[i]->setPositionAndVelocity(newPosition, 0);
            channels[i]->setIsActive(true);
        }
    }

    return true;
}

