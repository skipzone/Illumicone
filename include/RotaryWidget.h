#ifndef ROTARY_WIDGET_H
#define ROTARY_WIDGET_H

#include "Widget.h"

class RotaryWidget : public Widget
{
    public:
        RotaryWidget() {};
        ~RotaryWidget() {};

        RotaryWidget(const RotaryWidget&) = delete;
        RotaryWidget& operator = (const RotaryWidget&) = delete;
        bool moveData();
};

#endif /* ROTARY_WIDGET_H */
