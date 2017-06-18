/*
    This file is part of Illumicone.

    Illumicone is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Illumicone is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Illumicone.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <chrono>
///#include <stdbool.h>
///#include <iostream>
///#include <vector>

#include <time.h>

#include "WidgetChannel.h"

//using namespace std;


WidgetChannel::WidgetChannel(unsigned int channelNumber, Widget* widget, unsigned int autoInactiveMs)
    : channelNumber(channelNumber)
    , widget(widget)
    , isActive(false)
    , autoInactiveMs(autoInactiveMs)
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
        if (autoInactiveMs != 0) {
            using namespace std::chrono;
            milliseconds epochMs = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
            unsigned int nowMs = epochMs.count();
            if (nowMs - lastActiveMs > autoInactiveMs) {
                isActive = false;
            }
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
    if (isNowActive && autoInactiveMs != 0) {
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


