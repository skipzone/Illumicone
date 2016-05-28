#ifndef SUB_WIDGET_H
#define SUB_WIDGET_H

class WidgetChannel
{
    public:
        int     position;
        int     velocity;
        bool    isActive;

        bool init(int initPosition, int initVelocity) {
            position = initPosition;
            velocity = initVelocity;
            isActive = false;
            return true;
        }
};

#endif /* SUB_WIDGET_H */
