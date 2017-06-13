#pragma once

#include "Widget.h"
#include "WidgetChannel.h"

class PlungerWidget : public Widget
{
    public:
        PlungerWidget();
        ~PlungerWidget() {};

        PlungerWidget(const PlungerWidget&) = delete;
        PlungerWidget& operator =(const PlungerWidget&) = delete;

        bool init(ConfigReader& config);

        bool moveData();

    private:

        unsigned int lastUpdateMs[8];
        unsigned int updateIntervalMs[8];
};

