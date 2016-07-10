#ifndef WIDGET_FACTORY_H
#define WIDGET_FACTORY_H

#include "ThreeWheelWidget.h"
#include "EyeWidget.h"
#include "StairWidget.h"
#include "RainstickWidget.h"
#include "RotaryWidget.h"

using namespace std;


enum class WidgetId {
    reserved = 0,
    eye,
    hypnotyzer,
    bells,
    steps,
    rainstick,
    triObelisk,
    boxTheramin,
    plunger
};


static Widget* widgetFactory(WidgetId id)
{
    switch (id) {
        case WidgetId::eye:
            cout << "Return EyeWidget!" << endl;
            return new EyeWidget;
        case WidgetId::hypnotyzer:
            cout << "Return RotaryWidget!" << endl;
            return new RotaryWidget;
        case WidgetId::steps:
            cout << "Return StairWidget!" << endl;
            return new StairWidget;
        case WidgetId::triObelisk:
            cout << "Return ThreeWheelWidget!" << endl;
            return new ThreeWheelWidget;
        default:
            cout << "SOMETHING'S FUCKY: WidgetFactory id" << endl;
            return nullptr;
    }
}

#endif /* WIDGET_FACTORY_H */
