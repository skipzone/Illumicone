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

#include <string>


class Widget;


class WidgetChannel
{
    public:

        WidgetChannel(unsigned int channelNumber, Widget* widget, unsigned int autoInactiveMs);
        virtual ~WidgetChannel() {}

        WidgetChannel() = delete;
        WidgetChannel(const WidgetChannel&) = delete;
        WidgetChannel& operator =(const WidgetChannel&) = delete;

        
        unsigned int getChannelNumber();
        std::string getName();
        bool getIsActive();
        bool getHasNewPositionMeasurement();
        bool getHasNewVelocityMeasurement();
        int getPosition();
        int getVelocity();
        int getPreviousPosition();
        int getPreviousVelocity();

        void setIsActive(bool isNowActive);
        void setPosition(int newPosition);
        void setVelocity(int newVelocity);
        void setPositionAndVelocity(int newPosition, int newVelocity);

    private:

        const int channelNumber;
        Widget* widget;             // TODO:  not sure if we really need this
        bool isActive;
        unsigned int autoInactiveMs;
        unsigned int lastActiveMs;
        bool hasNewPositionMeasurement;
        bool hasNewVelocityMeasurement;
        int position;
        int velocity;
        int prevPosition;
        int prevVelocity;
};

