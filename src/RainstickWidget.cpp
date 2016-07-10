#include <chrono>
#include <iostream>
#include <string>
#include <time.h>

#include "RainstickWidget.h"
#include "illumiconeTypes.h"

using namespace std;


RainstickWidget::RainstickWidget()
{
    for (unsigned int i = 0; i < 8; ++i) {
        updateIntervalMs[i] = 0;
        lastUpdateMs[i] = 0;
    }

    updateIntervalMs[0] = 200;
}


void RainstickWidget::init()
{
    channels.push_back(make_shared<WidgetChannel>(0, this));
}


unsigned int RainstickWidget::getId()
{
    return RainstickWidget::id;
}


std::string RainstickWidget::getName()
{
    //return RainstickWidget::name;
    return "RainstickWidget";
}


bool RainstickWidget::moveData()
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


