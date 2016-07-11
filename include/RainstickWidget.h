
#pragma once

#include "Widget.h"
#include "WidgetChannel.h"

class RainstickWidget : public Widget
{
    public:
        RainstickWidget();
        ~RainstickWidget() {};

        RainstickWidget(const RainstickWidget&) = delete;
        RainstickWidget& operator =(const RainstickWidget&) = delete;

        void init(bool generateSimulatedMeasurements);

        unsigned int getId();
        std::string getName();

        bool moveData();

    private:

        constexpr static unsigned int id = 5;
        constexpr static char name[] = "Rainstick";

        unsigned int lastUpdateMs[8];
        unsigned int updateIntervalMs[8];
};

