#pragma once

#include "Widget.h"
#include "WidgetChannel.h"

class RotaryWidget : public Widget
{
    public:
        RotaryWidget();
        ~RotaryWidget() {};

        RotaryWidget(const RotaryWidget&) = delete;
        RotaryWidget& operator =(const RotaryWidget&) = delete;

        void init();

        unsigned int getId();
        std::string getName();

        bool moveData();

    private:

        constexpr static unsigned int id = 2;
        constexpr static char name[] = "Rotary";

        unsigned int lastUpdateMs[8];
        unsigned int updateIntervalMs[8];
};

