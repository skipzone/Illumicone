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

/* Widget Id Assignment
 *
 *  0:  reserved
 *  1:  Ray's Eye
 *  2:  Reiley's Hypnotyzer (the bike wheel)
 *  3:  Ray's Bells
 *  4:  Kelli's Steps
 *  5:  Naked's Rain Stick
 *  6:  Phyxx's TriObelisk (the triple rotary thing)
 *  7:  Charlie's Xylophone
 *  8:  Kayla's Plunger
 *  9:  Ross's FourPlay
 * 10:  Ray's and Mishi's Twister
 * 11:  Nick's Chairs
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
    xylophone,
    plunger,
    fourPlay,
    twister,
    chairs,
    invalid = 255
};


WidgetId intToWidgetId(unsigned int widgetIdValue);
unsigned int  widgetIdToInt(WidgetId widgetId);

