#pragma once

#include "Widget.h"
#include "WidgetChannel.h"

class ThreeWheelWidget : public Widget
{
    public:
        ThreeWheelWidget() {};
        ~ThreeWheelWidget() {};

        ThreeWheelWidget(const ThreeWheelWidget&) = delete;
        ThreeWheelWidget& operator =(const ThreeWheelWidget&) = delete;

        void init();

        unsigned int getId();
        std::string getName();

        bool moveData();

    private:

        constexpr static unsigned int id = 6;
        constexpr static char name[] = "Obelisk";

};

