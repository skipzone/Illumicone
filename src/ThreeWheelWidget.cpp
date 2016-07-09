#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <regex>

#include "ThreeWheelWidget.h"
#include "illumiconeTypes.h"

using namespace std;


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
    for (auto&& channel : channels) {
        int newPosition = (channel->getPreviousPosition() + channel->getChannelNumber()) % NUM_STRINGS;
        channel->setPositionAndVelocity(newPosition, 0);
    }

    return true;
}

