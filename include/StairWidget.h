#pragma once

#include "Widget.h"
#include "WidgetChannel.h"

class StairWidget : public Widget
{
    public:
        StairWidget();
        ~StairWidget() {};

        StairWidget(const StairWidget&) = delete;
        StairWidget& operator = (const StairWidget&) = delete;

        void init(bool generateSimulatedMeasurements);

        bool moveData();

    private:

        unsigned int lastUpdateMs[8];
        unsigned int updateIntervalMs[8];
};
