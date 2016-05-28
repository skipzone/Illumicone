#ifndef THREE_WHEEL_WIDGET_H
#define THREE_WHEEL_WIDGET_H

#include "Widget.h"

class ThreeWheelWidget : public Widget
{
    public:
        ThreeWheelWidget() {};
        ~ThreeWheelWidget() {};

        ThreeWheelWidget(const ThreeWheelWidget&) = delete;
        ThreeWheelWidget& operator =(const ThreeWheelWidget&) = delete;
        bool moveData();
};

#endif /* THREE_WHEEL_WIDGET_H */
