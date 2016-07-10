#ifndef WIDGET_FACTORY_H
#define WIDGET_FACTORY_H

#include "ThreeWheelWidget.h"
//#include "EyeWidget.h"
#include "StairWidget.h"
//#include "RainstickWidget.h"
//#include "RotaryWidget.h"

using namespace std;

static Widget* widgetFactory(uint8_t id)
{
    switch (id) {
/*
        case 1:
            cout << "Return ThreeWheelWidget!" << endl;
            return new ThreeWheelWidget;
        case 2:
            cout << "Return EyeWidget!" << endl;
            return new EyeWidget;
        case 3:
            cout << "Return StairWidget!" << endl;
            return new StairWidget;
        case 4:
            cout << "Return RotaryWidget!" << endl;
            return new RotaryWidget;
*/
        case 5:
            cout << "Return StairWidget!" << endl;
            return new StairWidget;
        case 6:
            cout << "Return ThreeWheelWidget!" << endl;
            return new ThreeWheelWidget;

        default:
            cout << "SOMETHING'S FUCKY: WidgetFactory id" << endl;
            return NULL;
    }
}

#endif /* WIDGET_FACTORY_H */
