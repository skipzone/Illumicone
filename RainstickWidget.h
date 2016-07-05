#ifndef RAINSTICK_WIDGET_H
#define RAINSTICK_WIDGET_H

#include "Widget.h"

class RainstickWidget : public Widget
{
    public:
        RainstickWidget() {};
        ~RainstickWidget() {};

        RainstickWidget(const RainstickWidget&) = delete;
        RainstickWidget& operator = (const RainstickWidget&) = delete;
        bool moveData();
};

#endif /* RAINSTICK_WIDGET_H */
