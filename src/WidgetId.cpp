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
            return WidgetId::shirleysWeb;
        case 3:
            return WidgetId::bells;
        case 4:
            return WidgetId::rainstick;
        case 5:
            return WidgetId::schroedersPlaything;
        case 6:
            return WidgetId::triObelisk;
        case 7:
            return WidgetId::squawkBox;
        case 8:
            return WidgetId::plunger;
        case 9:
            return WidgetId::contortOMatic;
        case 10:
            return WidgetId::fourPlay42;
        case 11:
            return WidgetId::fourPlay43;
        case 12:
            return WidgetId::buckNorris;
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
        case WidgetId::shirleysWeb:
            return 2;
        case WidgetId::bells:
            return 3;
        case WidgetId::rainstick:
            return 4;
        case WidgetId::schroedersPlaything:
            return 5;
        case WidgetId::triObelisk:
            return 6;
        case WidgetId::squawkBox:
            return 7;
        case WidgetId::plunger:
            return 8;
        case WidgetId::contortOMatic:
            return 9;
        case WidgetId::fourPlay42:
            return 10;
        case WidgetId::fourPlay43:
            return 11;
        case WidgetId::buckNorris:
            return 12;
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
        case WidgetId::shirleysWeb:
            return "shirleysWeb";
        case WidgetId::bells:
            return "bells";
        case WidgetId::rainstick:
            return "rainstick";
        case WidgetId::schroedersPlaything:
            return "schroedersPlaything";
        case WidgetId::triObelisk:
            return "triObelisk";
        case WidgetId::squawkBox:
            return "squawkBox";
        case WidgetId::plunger:
            return "plunger";
        case WidgetId::contortOMatic:
            return "contortOMatic";
        case WidgetId::fourPlay42:
            return "fourPlay42";
        case WidgetId::fourPlay43:
            return "fourPlay43";
        case WidgetId::buckNorris:
            return "buckNorris";
        case WidgetId::invalid:
            return "invalid";
    }
    return "invalid";
}


WidgetId stringToWidgetId(const std::string& widgetName)
{
    for (unsigned int i = 0; i <= 15; ++i) {
        WidgetId widgetId = intToWidgetId(i);
        if (widgetName == widgetIdToString(widgetId)) {
            return widgetId;
        }
    }
    return WidgetId::invalid;
}

