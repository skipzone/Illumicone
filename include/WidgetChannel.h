#ifndef SUB_WIDGET_H
#define SUB_WIDGET_H

class WidgetChannel
{
    public:
        int     prevPosition;
        int     position;
        int     velocity;
        int     isActive;
        int     number;

        bool initChannel(int channelNumber, int initPosition, int initVelocity);
};

#endif /* SUB_WIDGET_H */
