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


template<typename CollectionType, typename PixelType>
void fillSolid(CollectionType& collection, const PixelType& color)
{
    for (auto&& e : collection) {
        // We're counting on CPixelView's assignment operator when e is a pixel set.
        e = color;
    }
}

extern template void fillSolid(HsvPixelString&, const HsvPixel&);
extern template void fillSolid(RgbPixelString&, const RgbPixel&);
extern template void fillSolid(HsvConeStrings&, const HsvPixel&);
extern template void fillSolid(RgbConeStrings&, const RgbPixel&);


template<typename CollectionType, typename PixelType>
void fillSolid(CollectionType& collection, unsigned int idx, const PixelType& color)
{
    collection[idx] = color;
}

extern template void fillSolid(HsvConeStrings&, unsigned int, const HsvPixel&);
extern template void fillSolid(RgbConeStrings&, unsigned int, const RgbPixel&);

//void fillSolid(HsvPixelString& pixelString, const HsvPixel& color);
//void fillSolid(HsvConeStrings& coneStrings, unsigned int stringIdx, const HsvPixel& color);
//void fillSolid(HsvConeStrings& coneStrings, const HsvPixel& color);


void clearAllPixels(HsvConeStrings& coneStrings);
void clearAllPixels(RgbConeStrings& coneStrings);

//void hsv2rgb(const HsvConeStrings& coneStrings, std::vector<std::vector<CRGB>>& pixelArray);
void hsv2rgb(const HsvConeStrings& hsvConeStrings, RgbConeStrings& rgbConeStrings);


void rgb2hsv(const RgbPixel& rgb, HsvPixel& hsv);
void rgb2hsv(const RgbConeStrings& rgbConeStrings, HsvConeStrings& hsvConeStrings);


bool stringToHsvPixel(const std::string& hsvString, HsvPixel& hsvPixel);

void hsvPixelToString(const HsvPixel& hsvPixel, std::string& hsvString);


template<typename ConeStringsType, typename PixelStringType, typename PixelType>
bool allocateConePixels(ConeStringsType& coneStrings, int numStrings, int pixelsPerString)
{
    // Resize the colleciton of strings to match the number of strings.
    coneStrings.resize(numStrings, PixelStringType(nullptr, 0));

    // Allocate the pixels for each string.
    for (auto&& pixelString : coneStrings) {
        PixelType* newStringPixels = new PixelType[pixelsPerString];
        if (newStringPixels == 0) {
            return false;
        }
        pixelString.resize(newStringPixels, pixelsPerString);
    }

    clearAllPixels(coneStrings);

    return true;
}

extern template bool allocateConePixels<HsvConeStrings, HsvPixelString, HsvPixel>(HsvConeStrings&, int, int);
extern template bool allocateConePixels<RgbConeStrings, RgbPixelString, RgbPixel>(RgbConeStrings&, int, int);


template<typename ConeStringsType, typename PixelType>
void freeConePixels(ConeStringsType& coneStrings)
{
    for (auto&& pixelString : coneStrings) {
        delete [] (PixelType*) pixelString;
        pixelString.resize(nullptr, 0);
    }
}

extern template void freeConePixels<HsvConeStrings, HsvPixel>(HsvConeStrings&);
extern template void freeConePixels<RgbConeStrings, RgbPixel>(RgbConeStrings&);


// XY is used in two-dimensional filter functions.  See colorutils.cpp ported from FastLED.
uint16_t XY(uint8_t, uint8_t);


