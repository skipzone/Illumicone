#ifndef WIDGET_H
#define WIDGET_H

#include "WidgetChannel.h"

class Widget
{
    public:
        Widget(int, int, int);
        std::vector<WidgetChannel> channels;
};

#endif /* WIDGET_H */