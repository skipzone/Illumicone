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

using namespace std;


ThreeWheelWidget::ThreeWheelWidget()
{
    for (unsigned int i = 0; i < 8; ++i) {
        updateIntervalMs[i] = 0;
        lastUpdateMs[i] = 0;
    }

    updateIntervalMs[0] = 200;
    updateIntervalMs[1] = 400;
    updateIntervalMs[2] = 600;
}


void ThreeWheelWidget::init()
{
    for (int i = 0; i < 3; ++i) {
        channels.push_back(make_shared<WidgetChannel>(i, this));
    }
}


unsigned int ThreeWheelWidget::getId()
{
    return ThreeWheelWidget::id;
}


std::string ThreeWheelWidget::getName()
{
    //return ThreeWheelWidget::name;
    return "ThreeWheelWidget";
}


//
// Usually, this function will call each channel's update() function, then
// read the position, velocity, and isActive.  But, since the 3-wheel
// provides a string from UART that contains information about all 3 wheels, we
// just parse that instead.
//
bool ThreeWheelWidget::moveData()
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

