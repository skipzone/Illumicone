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

        void init();

        unsigned int getId();
        std::string getName();

        bool moveData();

    private:

        constexpr static unsigned int id = 5;
        constexpr static char name[] = "Stairs";

        unsigned int lastUpdateMs[8];
        unsigned int updateIntervalMs[8];
};
