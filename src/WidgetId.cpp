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
            return WidgetId::hypnotyzer;
        case 3:
            return WidgetId::bells;
        case 4:
            return WidgetId::steps;
        case 5:
            return WidgetId::rainstick;
        case 6:
            return WidgetId::triObelisk;
        case 7:
            return WidgetId::xylophone;
        case 8:
            return WidgetId::plunger;
        case 9:
            return WidgetId::fourPlay;
        case 10:
            return WidgetId::twister;
        case 11:
            return WidgetId::chairs;
        default:
            return WidgetId::invalid;
    }
    return WidgetId::invalid;
}


unsigned int  widgetIdToInt(WidgetId widgetId)
{
    switch (widgetId) {
        case WidgetId::reserved:
            return 0;
        case WidgetId::eye:
            return 1;
        case WidgetId::hypnotyzer:
            return 2;
        case WidgetId::bells:
            return 3;
        case WidgetId::steps:
            return 4;
        case WidgetId::rainstick:
            return 5;
        case WidgetId::triObelisk:
            return 6;
        case WidgetId::xylophone:
            return 7;
        case WidgetId::plunger:
            return 8;
        case WidgetId::fourPlay:
            return 9;
        case WidgetId::twister:
            return 10;
        case WidgetId::chairs:
            return 11;
        case WidgetId::invalid:
            return 255;
    }
    return 255;
}

