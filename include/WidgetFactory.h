#ifndef WIDGET_FACTORY_H
#define WIDGET_FACTORY_H

#include "ThreeWheelWidget.h"
#include "EyeWidget.h"
#include "StairWidget.h"

using namespace std;

static Widget* widgetFactory(uint8_t id)
{
    switch (id) {
        case 1:
            cout << "Return ThreeWheelWidget!" << endl;
            return new ThreeWheelWidget;
        case 2:
            cout << "Return SolidBlackWidget!" << endl;
            return new EyeWidget;
        case 3:
            cout << "Return StairWidget!" << endl;
            return new StairWidget;
        default:
            cout << "SOMETHING'S FUCKY: WidgetFactory id" << endl;
    }
}

#endif /* WIDGET_FACTORY_H */
