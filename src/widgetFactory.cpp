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


#include "BatonWidget.h"
#include "BellsWidget.h"
#include "BoogieBoardWidget.h"
#include "EyeWidget.h"
#include "FlowerWidget.h"
#include "FourPlay4xWidget.h"
#include "Log.h"
#include "PumpWidget.h"
#include "RainstickWidget.h"
#include "SchroedersPlaythingWidget.h"
#include "SpinnahWidget.h"
#include "widgetFactory.h"


extern Log logger;


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
        case WidgetId::schroedersPlaything:
            return new SchroedersPlaythingWidget;
        case WidgetId::pump:
            return new PumpWidget;
        case WidgetId::boogieBoard:
            return new BoogieBoardWidget;
        case WidgetId::fourPlay42:
        case WidgetId::fourPlay43:
            return new FourPlay4xWidget(id);
        case WidgetId::baton1:
        case WidgetId::baton2:
        case WidgetId::baton3:
        case WidgetId::baton4:
        case WidgetId::baton5:
        case WidgetId::baton6:
            return new BatonWidget(id);
        case WidgetId::flower1:
        case WidgetId::flower2:
        case WidgetId::flower3:
        case WidgetId::flower4:
        case WidgetId::flower5:
        case WidgetId::flower6:
        case WidgetId::flower7:
        case WidgetId::flower8:
            return new FlowerWidget(id);
        default:
            logger.logMsg(LOG_ERR, "Unsupported id passed to widgetFactory.");
            return nullptr;
    }
}

