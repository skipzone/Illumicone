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

/* Widget Id Assignment
 *
 *  0:  reserved
 *  1:  Eye (Ray)
 *  2:  Shirley's Web (Reiley, Monty)
 *  3:  Bells (Ray)
 *  4:  Rainstick (Dr. Naked)
 *  5:  Schroeder's Plaything (Ross)
 *  6:  TriObelisk (Phyxx and Dr. Naked)
 *  7:  Squawk Box (James)
 *  8:  Plunger (Monty, based on Kayla's original)
 *  9:  Twister (Ray and Mishi)
 * 10:  FourPlay-4-2 (Dr. Naked and Ross)
 * 11:  FourPlay-4-3 (Dr. Naked and Ross)
 * 12:  Buck Norris (Connie)
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
    shirleysWeb,
    bells,
    rainstick,
    schroedersPlaything,
    triObelisk,
    squawkBox,
    plunger,
    twister,
    fourPlay42,
    fourPlay43,
    buckNorris,
    invalid = 255
};


WidgetId intToWidgetId(unsigned int widgetIdValue);
unsigned int  widgetIdToInt(WidgetId widgetId);
std::string widgetIdToString(WidgetId widgetId);
WidgetId stringToWidgetId(const std::string& widgetName);

