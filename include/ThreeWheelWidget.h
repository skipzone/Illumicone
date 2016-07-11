#pragma once

#include "Widget.h"
#include "WidgetChannel.h"

class ThreeWheelWidget : public Widget
{
    public:
        ThreeWheelWidget();
        ~ThreeWheelWidget() {};

        ThreeWheelWidget(const ThreeWheelWidget&) = delete;
        ThreeWheelWidget& operator =(const ThreeWheelWidget&) = delete;

        void init(bool generateSimulatedMeasurements);

        bool moveData();

    private:

        unsigned int lastUpdateMs[8];
        unsigned int updateIntervalMs[8];
};

