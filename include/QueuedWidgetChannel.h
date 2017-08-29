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

#pragma once

#include <mutex>
#include <queue>
#include <string>

#include "WidgetChannel.h"


class Widget;


class QueuedWidgetChannel : public WidgetChannel
{
    public:

        QueuedWidgetChannel(unsigned int channelNumber, Widget* widget, unsigned int autoInactiveMs);
        virtual ~QueuedWidgetChannel() {}

        QueuedWidgetChannel() = delete;
        QueuedWidgetChannel(const QueuedWidgetChannel&) = delete;
        QueuedWidgetChannel& operator =(const QueuedWidgetChannel&) = delete;

        int getPosition();
        int getVelocity();

        void setPosition(int newPosition);
        void setVelocity(int newVelocity);
        void setPositionAndVelocity(int newPosition, int newVelocity);

    private:

        std::queue<int> positionQueue;
        std::queue<int> velocityQueue;
        std::mutex measmtQueueMutex;

};

