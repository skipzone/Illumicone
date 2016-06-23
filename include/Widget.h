#ifndef WIDGET_H
#define WIDGET_H

#include "WidgetChannel.h"
#include <vector>

class Widget
{
    public:
        Widget() {}
        virtual ~Widget() {}
        std::vector<WidgetChannel> channels;

        Widget(const Widget&) = delete;
        Widget& operator =(const Widget&) = delete;

        bool init(int numChannels) {
            channels.resize(numChannels);
            return true;
        }

        //
        // use this to read data from widget and store it
        //
        virtual bool moveData() = 0;

        //
        // use this to check if the widget is active before
        // calling Pattern::update
        //
//        virtual bool isActive();
};

#endif /* WIDGET_H */
