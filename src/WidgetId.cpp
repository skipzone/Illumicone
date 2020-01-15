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

#include "WidgetId.h"


WidgetId intToWidgetId(unsigned int widgetIdValue)
{
    switch (widgetIdValue) {
        case 0:
            return WidgetId::reserved;
        case 1:
            return WidgetId::eye;
        case 2:
            return WidgetId::spinnah;
        case 3:
            return WidgetId::bells;
        case 4:
            return WidgetId::rainstick;
        case 5:
            return WidgetId::schroedersPlaything;
        case 6:
            return WidgetId::pump;
        case 7:
            return WidgetId::boogieBoard;
        case 8:
            return WidgetId::fourPlay42;
        case 9:
            return WidgetId::fourPlay43;
        case 10:
            return WidgetId::invalid;
        case 11:
            return WidgetId::flower1;
        case 12:
            return WidgetId::flower2;
        case 13:
            return WidgetId::flower3;
        case 14:
            return WidgetId::flower4;
        case 15:
            return WidgetId::flower5;
        case 16:
            return WidgetId::flower6;
        case 17:
            return WidgetId::flower7;
        case 18:
            return WidgetId::invalid;
        case 19:
            return WidgetId::invalid;
        case 20:
            return WidgetId::invalid;
        case 21:
            return WidgetId::baton1;
        case 22:
            return WidgetId::baton2;
        case 23:
            return WidgetId::baton3;
        case 24:
            return WidgetId::baton4;
        case 25:
            return WidgetId::baton5;
        case 26:
            return WidgetId::baton6;
        case 27:
            return WidgetId::lidar1;
        case 28:
            return WidgetId::invalid;
        case 29:
            return WidgetId::invalid;
        case 30:
            return WidgetId::invalid;
        case 31:
            return WidgetId::invalid;
        default:
            return WidgetId::invalid;
    }
    return WidgetId::invalid;
}


unsigned int widgetIdToInt(WidgetId widgetId)
{
    switch (widgetId) {
        case WidgetId::reserved:
            return 0;
        case WidgetId::eye:
            return 1;
        case WidgetId::spinnah:
            return 2;
        case WidgetId::bells:
            return 3;
        case WidgetId::rainstick:
            return 4;
        case WidgetId::schroedersPlaything:
            return 5;
        case WidgetId::pump:
            return 6;
        case WidgetId::boogieBoard:
            return 7;
        case WidgetId::fourPlay42:
            return 8;
        case WidgetId::fourPlay43:
            return 9;
        //case WidgetId:::
        //    return 10;
        case WidgetId::flower1:
            return 11;
        case WidgetId::flower2:
            return 12;
        case WidgetId::flower3:
            return 13;
        case WidgetId::flower4:
            return 14;
        case WidgetId::flower5:
            return 15;
        case WidgetId::flower6:
            return 16;
        case WidgetId::flower7:
            return 17;
        //case WidgetId:::
        //    return 18;
        //case WidgetId:::
        //    return 19;
        //case WidgetId:::
        //    return 20;
        case WidgetId::baton1:
            return 21;
        case WidgetId::baton2:
            return 22;
        case WidgetId::baton3:
            return 23;
        case WidgetId::baton4:
            return 24;
        case WidgetId::baton5:
            return 25;
        case WidgetId::baton6:
            return 26;
        case WidgetId::lidar1:
            return 27;
        //case WidgetId:::
        //    return 28;
        //case WidgetId:::
        //    return 29;
        //case WidgetId:::
        //    return 30;
        //case WidgetId:::
        //    return 31;
        case WidgetId::invalid:
            return 255;
    }
    return 255;
}


std::string widgetIdToString(WidgetId widgetId)
{
    switch (widgetId) {
        case WidgetId::reserved:
            return "reserved";
        case WidgetId::eye:
            return "eye";
        case WidgetId::spinnah:
            return "spinnah";
        case WidgetId::bells:
            return "bells";
        case WidgetId::rainstick:
            return "rainstick";
        case WidgetId::schroedersPlaything:
            return "schroedersPlaything";
        case WidgetId::pump:
            return "pump";
        case WidgetId::boogieBoard:
            return "boogieBoard";
        case WidgetId::fourPlay42:
            return "fourPlay42";
        case WidgetId::fourPlay43:
            return "fourPlay43";
        case WidgetId::flower1:
            return "flower1";
        case WidgetId::flower2:
            return "flower2";
        case WidgetId::flower3:
            return "flower3";
        case WidgetId::flower4:
            return "flower4";
        case WidgetId::flower5:
            return "flower5";
        case WidgetId::flower6:
            return "flower6";
        case WidgetId::flower7:
            return "flower7";
        case WidgetId::baton1:
            return "baton1";
        case WidgetId::baton2:
            return "baton2";
        case WidgetId::baton3:
            return "baton3";
        case WidgetId::baton4:
            return "baton4";
        case WidgetId::baton5:
            return "baton5";
        case WidgetId::baton6:
            return "baton6";
        case WidgetId::lidar1:
            return "lidar1";
        case WidgetId::invalid:
            return "invalid";
    }
    return "invalid";
}


WidgetId stringToWidgetId(const std::string& widgetName)
{
    for (unsigned int i = 0; i <= 31; ++i) {
        WidgetId widgetId = intToWidgetId(i);
        if (widgetName == widgetIdToString(widgetId)) {
            return widgetId;
        }
    }
    return WidgetId::invalid;
}

