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


#include "BellsWidget.h"
//#include "BuckNorrisWidget.h"
#include "EyeWidget.h"
#include "FourPlay42Widget.h"
#include "FourPlay43Widget.h"
#include "log.h"
#include "PumpWidget.h"
#include "RainstickWidget.h"
//#include "SchroedersPlaythingWidget.h"
#include "SpinnahWidget.h"
#include "TriObeliskWidget.h"
//#include "ContortOMaticWidget.h"
#include "widgetFactory.h"


Widget* widgetFactory(WidgetId id) {
    switch (id) {
        case WidgetId::eye:
            return new EyeWidget;
        case WidgetId::spinnah:
            return new SpinnahWidget;
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
        case WidgetId::pump:
            return new PumpWidget;
//        case WidgetId::contortOMatic:
//            return new ContortOMaticWidget;
        case WidgetId::fourPlay42:
            return new FourPlay42Widget;
        case WidgetId::fourPlay43:
            return new FourPlay43Widget;
//        case WidgetId::buckNorris:
//            return new BuckNorrisWidget;
        default:
            logMsg(LOG_ERR, "Unsupported id passed to widgetFactory.");
            return nullptr;
    }
}
