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


template void fillSolid(HsvPixelString&, const HsvPixel&);
template void fillSolid(RgbPixelString&, const RgbPixel&);
template void fillSolid(HsvConeStrings&, const HsvPixel&);
template void fillSolid(RgbConeStrings&, const RgbPixel&);


template void fillSolid(HsvConeStrings&, unsigned int, const HsvPixel&);
template void fillSolid(RgbConeStrings&, unsigned int, const RgbPixel&);


void clearAllPixels(HsvConeStrings& coneStrings)
{
    HsvPixel transparent(0, 0, 0);
    fillSolid(coneStrings, transparent);
}


void clearAllPixels(RgbConeStrings& coneStrings)
{
    RgbPixel transparent(0, 0, 0);
    fillSolid(coneStrings, transparent);
}


void hsv2rgb(const HsvConeStrings& hsvConeStrings, RgbConeStrings& rgbConeStrings)
{
    unsigned int numStrings = std::min(hsvConeStrings.size(), rgbConeStrings.size());
    for (unsigned int i = 0; i < numStrings; ++i) {
        unsigned int numPixels = std::min(hsvConeStrings[i].size(), rgbConeStrings[i].size());
        hsv2rgb_rainbow((HsvPixel*) hsvConeStrings[i], (RgbPixel*) rgbConeStrings[i], numPixels);
    }
}


void rgb2hsv(const RgbConeStrings& rgbConeStrings, HsvConeStrings& hsvConeStrings)
{
    unsigned int numStrings = std::min(hsvConeStrings.size(), rgbConeStrings.size());
    for (unsigned int i = 0; i < numStrings; ++i) {
        unsigned int numPixels = std::min(hsvConeStrings[i].size(), rgbConeStrings[i].size());
        for (unsigned int j = 0; j < numPixels; ++j) {
            hsvConeStrings[i][j] = rgb2hsv_approximate(rgbConeStrings[i][j]);
        }
    }
}


template bool allocateConePixels<HsvConeStrings, HsvPixelString, HsvPixel>(HsvConeStrings&, int, int);
template bool allocateConePixels<RgbConeStrings, RgbPixelString, RgbPixel>(RgbConeStrings&, int, int);


template void freeConePixels<HsvConeStrings, HsvPixel>(HsvConeStrings&);
template void freeConePixels<RgbConeStrings, RgbPixel>(RgbConeStrings&);


// XY is used in two-dimensional filter functions.  See colorutils.cpp ported from FastLED.
uint16_t XY(uint8_t, uint8_t)
{
    // TODO 7/24/2017 ross:  Need to do one-time init with cone dimensions.  For now, just return zero.
    return 0;
}


