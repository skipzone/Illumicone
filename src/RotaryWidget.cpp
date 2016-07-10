#include <chrono>
#include <iostream>
#include <string>
//#include <thread>
#include <time.h>
///#include <vector>

#include "RotaryWidget.h"
#include "illumiconeTypes.h"

using namespace std;


RotaryWidget::RotaryWidget()
{
    for (unsigned int i = 0; i < 8; ++i) {
        updateIntervalMs[i] = 0;
        lastUpdateMs[i] = 0;
    }

    updateIntervalMs[0] = 100;
}


void RotaryWidget::init()
{
    channels.push_back(make_shared<WidgetChannel>(0, this));
}


unsigned int RotaryWidget::getId()
{
    return RotaryWidget::id;
}


std::string RotaryWidget::getName()
{
    //return RotaryWidget::name;
    return "RotaryWidget";
}


//
// Usually, this function will call each channel's update() function, then
// read the position, velocity, and isActive.  But, since the 3-wheel
// provides a string from UART that contains information about all 3 wheels, we
// just parse that instead.
//
bool RotaryWidget::moveData()
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

