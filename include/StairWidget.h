#ifndef STAIR_WIDGET_H
#define STAIR_WIDGET_H

#include "Widget.h"

class StairWidget : public Widget
{
    public:
        StairWidget() {};
        ~StairWidget() {};

        StairWidget(const StairWidget&) = delete;
        StairWidget& operator = (const StairWidget&) = delete;
        bool moveData();
};

#endif /* STAIR_WIDGET_H */
