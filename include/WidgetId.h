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

#include <string>

constexpr int maxWidgets = 32;


/* Widget Id Assignment
 *
 *  0:  reserved
 *  1:  Eye (Ray)
 *  2:  Spinnah (Katya, Reiley, Monty)
 *  3:  Bells (Ray)
 *  4:  Rainstick (Dr. Naked)
 *  5:  Schroeder's Plaything (Ross)
 *  6:  Pump (Monte, Dr. Naked; based on Kayla's plunger)
 *  7:  BoogieBoard (Connie)
 *  8:  FourPlay-4-2 (Dr. Naked, Ross)
 *  9:  FourPlay-4-3 (Dr. Naked, Ross)
 * 10:  unassigned (reserved for AI mic)
 * 11:  flower #1
 * 12:  flower #2
 * 13:  flower #3
 * 14:  flower #4
 * 15:  flower #5
 * 16:  flower #6
 * 17:  flower #7
 * 18:  unassigned
 * 19:  unassigned (reserved for maraca #1)
 * 20:  unassigned (reserved for maraca #2)
 * 21:  Baton #1
 * 22:  Baton #2
 * 23:  Baton #3
 * 24:  Baton #4
 * 25:  Baton #5
 * 26:  Baton #6
 * 27:  Lidar1
 * 28:  unassigned
 * 29:  unassigned
 * 30:  unassigned
 * 31:  unassigned
 *
 * For stress tests, widget ids are reused as needed because stress-test
 * payloads are handled separately from all other types of payloads.
 */

enum class WidgetId {
    reserved = 0,
    eye,
    spinnah,
    bells,
    rainstick,
    schroedersPlaything,
    pump,
    boogieBoard,
    fourPlay42,
    fourPlay43,
    flower1 = 11,
    flower2,
    flower3,
    flower4,
    flower5,
    flower6,
    flower7,
    baton1 = 21,
    baton2,
    baton3,
    baton4,
    baton5,
    baton6,
    lidar1 = 27,
    invalid = 255
};


WidgetId intToWidgetId(unsigned int widgetIdValue);
unsigned int  widgetIdToInt(WidgetId widgetId);
std::string widgetIdToString(WidgetId widgetId);
WidgetId stringToWidgetId(const std::string& widgetName);

