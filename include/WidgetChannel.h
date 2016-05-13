#ifndef SUB_WIDGET_H
#define SUB_WIDGET_H

class WidgetChannel
{
    public:
        WidgetChannel(int, int);
        int     position;
        int     velocity;
        bool    isActive;
};

#endif /* SUB_WIDGET_H */
