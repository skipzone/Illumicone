#pragma once

/* Widget Id Assignment
 *
 *  0:  reserved
 *  1:  Ray's Eye
 *  2:  Reiley's Hypnotyzer (the bike wheel)
 *  3:  Ray's Bells
 *  4:  Kelli's Steps
 *  5:  Naked's Rain Stick
 *  6:  Phyxx's TriObelisk (the triple rotary thing)
 *  7:  Cowboy's Box Theramin
 *  8:  Kayla's Plunger
 *  9:  unassigned
 * 10:  unassigned
 * 11:  unassigned
 * 12:  unassigned
 * 13:  unassigned
 * 14:  unassigned
 * 15:  unassigned
 *
 * For stress tests, widget ids are reused as needed because stress-test
 * payloads are handled separately from all other types of payloads.
 */

enum class WidgetId {
    reserved = 0,
    eye,
    hypnotyzer,
    bells,
    steps,
    rainstick,
    triObelisk,
    boxTheramin,
    plunger,
    fourPlay,
    invalid = 255
};


WidgetId intToWidgetId(unsigned int widgetIdValue);
unsigned int  widgetIdToInt(WidgetId widgetId);

