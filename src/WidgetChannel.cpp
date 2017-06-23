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
#include <iostream>
///#include <vector>

#include <time.h>

#include "Widget.h"
#include "WidgetChannel.h"

using namespace std;


WidgetChannel::WidgetChannel(unsigned int channelNumber, Widget* widget, unsigned int autoInactiveMs)
    : channelNumber(channelNumber)
    , widget(widget)
    , isActive(false)
    , autoInactiveMs(autoInactiveMs)
    , lastActiveMs(0)
    , hasNewPositionMeasurement(false)
    , hasNewVelocityMeasurement(false)
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


string WidgetChannel::getName()
{
    return widgetIdToString(widget->getId()) + "/ch" + to_string(channelNumber);
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


bool WidgetChannel::getHasNewPositionMeasurement()
{
    return hasNewPositionMeasurement;
}


bool WidgetChannel::getHasNewVelocityMeasurement()
{
    return hasNewVelocityMeasurement;
}


int WidgetChannel::getPosition()
{
    if (hasNewPositionMeasurement) {
        hasNewPositionMeasurement = false;
        prevPosition = position;
    }
    return position;
}


int WidgetChannel::getVelocity()
{
    if (hasNewVelocityMeasurement) {
        hasNewVelocityMeasurement = false;
        prevVelocity = velocity;
    }
    return velocity;
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


// A race condition will exist if setPosition, setVelocity, or setPositionAndVelocity are called from anywhere
// other than Widget::pollForUdpRx() when simulated measurements are not being used because pollForUpdRx and
// the pattern controller run on different threads.  However, there is no reason to call any of these set
// functions when simulated measurements are not being used, so there really shouldn't be a problem.


void WidgetChannel::setPosition(int newPosition)
{
    //cout << "setting position to " << newPosition << endl;
    position = newPosition;
    hasNewPositionMeasurement = true;
}


void WidgetChannel::setVelocity(int newVelocity)
{
    //cout << "setting velocity to " << newVelocity << endl;
    velocity = newVelocity;
    hasNewVelocityMeasurement = true;
}


void WidgetChannel::setPositionAndVelocity(int newPosition, int newVelocity)
{
    //cout << "setting position to " << newPosition << ", velocity to " << newVelocity << endl;
    position = newPosition;
    velocity = newVelocity;
    hasNewPositionMeasurement = true;
    hasNewVelocityMeasurement = true;
}


