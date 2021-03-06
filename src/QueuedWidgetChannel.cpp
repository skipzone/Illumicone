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

#include <string>

#include "illumiconeUtility.h"
#include "Log.h"
#include "Widget.h"
#include "QueuedWidgetChannel.h"

using namespace std;


extern Log logger;


QueuedWidgetChannel::QueuedWidgetChannel(unsigned int channelNumber, Widget* widget, unsigned int autoInactiveMs)
    :WidgetChannel(channelNumber, widget, autoInactiveMs)
{
}


int QueuedWidgetChannel::getPosition()
{
    measmtQueueMutex.lock();
    if (!positionQueue.empty()) {
        prevPosition = position;
        position = positionQueue.front();
        positionQueue.pop();
        hasNewPositionMeasurement = !positionQueue.empty();
        //logger.logMsg(LOG_DEBUG, "positionQueue.size() returns " + to_string(positionQueue.size()) + ", .empty() returns " + to_string(positionQueue.empty()) + ", hasNewPositionMeasurement=" + to_string(hasNewPositionMeasurement));
    }
    measmtQueueMutex.unlock();
    return position;
}


int QueuedWidgetChannel::getVelocity()
{
    measmtQueueMutex.lock();
    if (!velocityQueue.empty()) {
        prevVelocity = velocity;
        velocity = velocityQueue.front();
        velocityQueue.pop();
        hasNewVelocityMeasurement = !velocityQueue.empty();
        //logger.logMsg(LOG_DEBUG, "velocityQueue.size() returns " + to_string(velocityQueue.size()) + ", .empty() returns " + to_string(velocityQueue.empty()) + ", hasNewVelocityMeasurement=" + to_string(hasNewVelocityMeasurement));
    }
    measmtQueueMutex.unlock();
    return velocity;
}


void QueuedWidgetChannel::setPosition(int newPosition)
{
    measmtQueueMutex.lock();
    positionQueue.push(newPosition);
    hasNewPositionMeasurement = true;
    measmtQueueMutex.unlock();
}


void QueuedWidgetChannel::setVelocity(int newVelocity)
{
    measmtQueueMutex.lock();
    velocityQueue.push(newVelocity);
    hasNewVelocityMeasurement = true;
    measmtQueueMutex.unlock();
}


void QueuedWidgetChannel::setPositionAndVelocity(int newPosition, int newVelocity)
{
    measmtQueueMutex.lock();
    positionQueue.push(newPosition);
    velocityQueue.push(newVelocity);
    hasNewPositionMeasurement = true;
    hasNewVelocityMeasurement = true;
    measmtQueueMutex.unlock();
}


