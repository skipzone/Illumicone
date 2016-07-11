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

        void init(bool generateSimulatedMeasurements);

        bool moveData();

    private:

        unsigned int lastUpdateMs[8];
        unsigned int updateIntervalMs[8];
};

