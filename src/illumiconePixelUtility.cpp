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

//#include <string>

#include "hsv2rgb.h"
#include "illumiconePixelUtility.h"
//#include "log.h"


bool allocateConePixels(HsvConeStrings& coneStrings, int pixelsPerString, int numStrings)
{
    // Resize the colleciton of strings to match the number of strings.
    coneStrings.resize(numStrings, HsvPixelString(nullptr, 0));

    // Allocate the pixels for each string.
    for (auto&& pixelString : coneStrings) {
        HsvPixel* newStringPixels = new HsvPixel[pixelsPerString];
        if (newStringPixels == 0) {
            return false;
        }
        pixelString.resize(newStringPixels, pixelsPerString);
    }

    clearAllPixels(coneStrings);

    return true;
}


void freeConePixels(HsvConeStrings& coneStrings)
{
    for (auto&& pixelString : coneStrings) {
        delete [] (HsvPixel*) pixelString;
        pixelString.resize(nullptr, 0);
    }
}


void fillSolid(HsvPixelString& pixelString, const HsvPixel& color)
{
    pixelString = color;
}


void fillSolid(HsvConeStrings& coneStrings, unsigned int stringIdx, const HsvPixel& color)
{
    coneStrings[stringIdx] = color;
}


void fillSolid(HsvConeStrings& coneStrings, const HsvPixel& color)
{
    for (auto&& pixelString : coneStrings) {
        pixelString = color;
    }
}


void clearAllPixels(HsvConeStrings& coneStrings)
{
    HsvPixel transparent = {0, 0, 0};
    fillSolid(coneStrings, transparent);
}


void hsv2rgb(const HsvConeStrings& coneStrings, std::vector<std::vector<CRGB>>& pixelArray)
{
    for (unsigned int i = 0; i < pixelArray.size(); ++i) {
        hsv2rgb_rainbow((HsvPixel*) coneStrings[i], pixelArray[i].data(), pixelArray[i].size());
    }
}


// XY is used in two-dimensional filter functions.  See colorutils.cpp ported from FastLED.
uint16_t XY(uint8_t, uint8_t)
{
    // TODO 7/24/2017 ross:  Need to do one-time init with cone dimensions.  For now, just return zero.
    return 0;
}


