#include <stdbool.h>
#include <iostream>
#include <vector>

#include "WidgetChannel.h"

using namespace std;

//WidgetChannel::WidgetChannel(int initialPosition, int initialVelocity) :
//    position(initialPosition),
//    velocity(initialVelocity),
//    isActive(false)
//{
//}
bool WidgetChannel::initChannel(int channelNumber, int initPosition, int initVelocity)
{
    prevPosition = initPosition;
    position = initPosition;
    velocity = initVelocity;
    number = channelNumber;
    isActive = 0;

    return true;
}

