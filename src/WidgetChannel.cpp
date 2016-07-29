#include <chrono>
///#include <stdbool.h>
///#include <iostream>
///#include <vector>

#include <time.h>

#include "WidgetChannel.h"

//using namespace std;


WidgetChannel::WidgetChannel(unsigned int channelNumber, Widget* widget)
    : channelNumber(channelNumber)
    , widget(widget)
    , isActive(false)
    , autoInactiveMs(2000)      // make configurable
    , lastActiveMs(0)
    , hasNewMeasurement(false)
    , position(0)
    , velocity(0)
    , prevPosition(0)
    , prevVelocity(0)
{
}


unsigned int WidgetChannel::getChannelNumber()
{
    return channelNumber;
}


bool WidgetChannel::getIsActive()
{
    if (isActive) {
        using namespace std::chrono;
        milliseconds epochMs = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
        unsigned int nowMs = epochMs.count();
        if (nowMs - lastActiveMs > autoInactiveMs) {
            isActive = false;
        }
    }

    return isActive;
}


bool WidgetChannel::getHasNewMeasurement()
{
    return hasNewMeasurement;
}


int WidgetChannel::getPosition()
{
    hasNewMeasurement = false;
    prevPosition = position;
    return position;
}


int WidgetChannel::getVelocity()
{
    hasNewMeasurement = false;
    prevVelocity = velocity;
    return velocity;
}


void WidgetChannel::positionAndVelocity(int& currentPosition, int& currentVelocity)
{
    hasNewMeasurement = false;
    prevPosition = position;
    prevVelocity = velocity;
    currentPosition = position;
    currentVelocity = velocity;
}


int WidgetChannel::getPreviousPosition()
{
    return prevPosition;
}


int WidgetChannel::getPreviousVelocity()
{
    return prevVelocity;
}


void WidgetChannel::setIsActive(bool isNowActive)
{
    if (isNowActive) {
        using namespace std::chrono;
        milliseconds epochMs = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
        lastActiveMs = epochMs.count();
    }

    isActive = isNowActive;
}


void WidgetChannel::setPosition(int newPosition)
{
    position = newPosition;
    hasNewMeasurement = true;
}


void WidgetChannel::setVelocity(int newVelocity)
{
    velocity = newVelocity;
    hasNewMeasurement = true;
}


void WidgetChannel::setPositionAndVelocity(int newPosition, int newVelocity)
{
    position = newPosition;
    velocity = newVelocity;
    hasNewMeasurement = true;
}


