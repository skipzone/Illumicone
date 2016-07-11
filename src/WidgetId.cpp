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
            return WidgetId::boxTheramin;
        case 8:
            return WidgetId::plunger;
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
        case WidgetId::boxTheramin:
            return 7;
        case WidgetId::plunger:
            return 8;
        case WidgetId::invalid:
            return 255;
    }
    return 255;
}

