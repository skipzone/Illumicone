#ifndef WIDGET_FACTORY_H
#define WIDGET_FACTORY_H

#include "ThreeWheelWidget.h"

static Widget* widgetFactory(uint8_t id)
{
    switch (id) {
        case 1:
            return new ThreeWheelWidget;
        default:
            return new ThreeWheelWidget;
    }
}

#endif /* WIDGET_FACTORY_H */
