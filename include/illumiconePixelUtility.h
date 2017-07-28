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

#include <stdint.h>
#include <vector>

#include "illumiconePixelTypes.h"


bool allocateConePixels(HsvConeStrings& coneStrings, int pixelsPerString, int numStrings);
void freeConePixels(HsvConeStrings& coneStrings);

void fillSolid(HsvPixelString& pixelString, const HsvPixel& color);
void fillSolid(HsvConeStrings& coneStrings, unsigned int stringIdx, const HsvPixel& color);
void fillSolid(HsvConeStrings& coneStrings, const HsvPixel& color);

void hsv2rgb(const HsvConeStrings& coneStrings, std::vector<std::vector<CRGB>>& pixelArray);


// XY is used in two-dimensional filter functions.  See colorutils.cpp ported from FastLED.
uint16_t XY(uint8_t, uint8_t);


