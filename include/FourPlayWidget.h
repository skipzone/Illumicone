#pragma once

#include "Widget.h"
#include "WidgetChannel.h"

class FourPlayWidget : public Widget
{
    public:
        FourPlayWidget();
        ~FourPlayWidget() {};

        FourPlayWidget(const FourPlayWidget&) = delete;
        FourPlayWidget& operator =(const FourPlayWidget&) = delete;

        void init(bool generateSimulatedMeasurements);

        bool moveData();

    private:

        unsigned int lastUpdateMs[8];
        unsigned int updateIntervalMs[8];
};

