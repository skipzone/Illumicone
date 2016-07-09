#pragma once

class Widget;

class WidgetChannel
{
    public:

        WidgetChannel(unsigned int channelNumber, Widget* widget);
        virtual ~WidgetChannel() {}

        WidgetChannel() = delete;
        WidgetChannel(const WidgetChannel&) = delete;
        WidgetChannel& operator =(const WidgetChannel&) = delete;

        unsigned int getChannelNumber();
        bool getIsActive();
        bool getHasNewMeasurement();
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
        bool hasNewMeasurement;
        int position;
        int velocity;
        void positionAndVelocity(int& position, int& velocity);
        int prevPosition;
        int prevVelocity;

///        bool initChannel(int channelNumber, int initPosition, int initVelocity);
};

