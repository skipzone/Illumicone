/*
    This file is part of Illumicone.

    Illumicone is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Illumicone is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Illumicone.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef WIDGET_FACTORY_H
#define WIDGET_FACTORY_H

#include "EyeWidget.h"
#include "RainstickWidget.h"
#include "RotaryWidget.h"
#include "StairWidget.h"
#include "ThreeWheelWidget.h"
#include "PlungerWidget.h"
#include "BellsWidget.h"
#include "FourPlayWidget.h"
#include "FourPlay42Widget.h"
#include "FourPlay43Widget.h"
#include "WidgetId.h"


using namespace std;


static Widget* widgetFactory(WidgetId id) { switch (id) {
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
            // TODO: CHANGE THIS TO RAINSTICK WHEN WIDGET FILE IS READY!
//        case WidgetId::rainstick:
//            cout << "Return RainstickWidget!" << endl;
//            return new PlungerWidget;
        case WidgetId::plunger:
            cout << "Return PlungerWidget!" << endl;
            return new PlungerWidget;
        case WidgetId::fourPlay:
            cout << "Return FourPlayWidget!" << endl;
            return new FourPlayWidget;
        case WidgetId::fourPlay42:
            cout << "Return FourPlay42Widget!" << endl;
            return new FourPlay42Widget;
        case WidgetId::fourPlay43:
            cout << "Return FourPlay43Widget!" << endl;
            return new FourPlay43Widget;
        case WidgetId::bells:
            cout << "Return BellsWidget!" << endl;
            return new BellsWidget;
        default:
            cout << "SOMETHING'S FUCKY: WidgetFactory id" << endl;
            return nullptr;
    }
}

#endif /* WIDGET_FACTORY_H */
