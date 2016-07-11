#ifndef WIDGET_FACTORY_H
#define WIDGET_FACTORY_H

#include "EyeWidget.h"
#include "RainstickWidget.h"
#include "RotaryWidget.h"
#include "StairWidget.h"
#include "ThreeWheelWidget.h"
#include "WidgetId.h"


using namespace std;


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
