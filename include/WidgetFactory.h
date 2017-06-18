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

#pragma once

#include "BellsWidget.h"
//#include "BuckNorrisWidget.h"
#include "EyeWidget.h"
#include "FourPlay42Widget.h"
#include "FourPlay43Widget.h"
#include "PlungerWidget.h"
#include "RainstickWidget.h"
//#include "SchroedersPlaythingWidget.h"
#include "ShirleysWebWidget.h"
//#include "SquawkBoxWidget.h"
#include "TriObeliskWidget.h"
//#include "TwisterWidget.h"
#include "WidgetId.h"


using namespace std;


static Widget* widgetFactory(WidgetId id) { switch (id) {
        case WidgetId::eye:
            return new EyeWidget;
        case WidgetId::shirleysWeb:
            return new ShirleysWebWidget;
        case WidgetId::bells:
            return new BellsWidget;
        case WidgetId::rainstick:
            return new RainstickWidget;
//        case WidgetId::schroedersPlaything:
//            return new SchroedersPlaythingWidget;
        case WidgetId::triObelisk:
            return new TriObeliskWidget;
//        case WidgetId::squawkBox:
//            return new SquawkBoxWidget;
        case WidgetId::plunger:
            return new PlungerWidget;
//        case WidgetId::twister:
//            return new TwisterWidget;
        case WidgetId::fourPlay42:
            return new FourPlay42Widget;
        case WidgetId::fourPlay43:
            return new FourPlay43Widget;
//        case WidgetId::buckNorris:
//            return new BuckNorrisWidget;
        default:
            cerr << "SOMETHING'S FUCKY: WidgetFactory id" << endl;
            return nullptr;
    }
}

