#ifndef EYE_WIDGET_H
#define EYE_WIDGET_H

#include "Widget.h"
#include "WidgetChannel.h"

class EyeWidget : public Widget
{
    public:
        EyeWidget();
        ~EyeWidget() {};

        EyeWidget(const EyeWidget&) = delete;
        EyeWidget& operator = (const EyeWidget&) = delete;

        void init(bool generateSimulatedMeasurements);

        bool moveData();

    private:

        unsigned int lastUpdateMs[8];
        unsigned int updateIntervalMs[8];
};

#endif /* EYE_WIDGET_H */
