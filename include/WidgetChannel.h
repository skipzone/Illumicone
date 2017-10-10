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

        
        virtual unsigned int getChannelNumber();
        virtual std::string getName();

        virtual bool getIsActive();
        virtual bool getHasNewPositionMeasurement();
        virtual bool getHasNewVelocityMeasurement();
        virtual int getPosition();
        virtual int getVelocity();
        virtual int getPreviousPosition();
        virtual int getPreviousVelocity();

        virtual void setIsActive(bool isNowActive);
        virtual void setPosition(int newPosition);
        virtual void setVelocity(int newVelocity);
        virtual void setPositionAndVelocity(int newPosition, int newVelocity);

    protected:

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

