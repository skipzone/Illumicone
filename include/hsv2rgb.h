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

    ----------------------------------------------------------------------------

    The contents of this file were ported from FastLED version 3.001.003.
    FastLED bears the following copyright message:

        The MIT License (MIT)

        Copyright (c) 2013 FastLED

        Permission is hereby granted, free of charge, to any person obtaining a copy of
        this software and associated documentation files (the "Software"), to deal in
        the Software without restriction, including without limitation the rights to
        use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
        the Software, and to permit persons to whom the Software is furnished to do so,
        subject to the following conditions:

        The above copyright notice and this permission notice shall be included in all
        copies or substantial portions of the Software.

        THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
        IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
        FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
        COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
        IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
        CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

    See the official FastLED site http://fastled.io for additional information.
*/

#pragma once

#include "pixeltypes.h"


// hsv2rgb_rainbow - convert a hue, saturation, and value to RGB
//                   using a visually balanced rainbow (vs a straight
//                   mathematical spectrum).
//                   This 'rainbow' yields better yellow and orange
//                   than a straight 'spectrum'.
//
//                   NOTE: here hue is 0-255, not just 0-191

void hsv2rgb_rainbow( const struct CHSV& hsv, struct CRGB& rgb);
void hsv2rgb_rainbow( const struct CHSV* phsv, struct CRGB * prgb, int numLeds);
#define HUE_MAX_RAINBOW 255


// hsv2rgb_spectrum - convert a hue, saturation, and value to RGB
//                    using a mathematically straight spectrum (vs
//                    a visually balanced rainbow).
//                    This 'spectrum' will have more green & blue
//                    than a 'rainbow', and less yellow and orange.
//
//                    NOTE: here hue is 0-255, not just 0-191

void hsv2rgb_spectrum( const struct CHSV& hsv, struct CRGB& rgb);
void hsv2rgb_spectrum( const struct CHSV* phsv, struct CRGB * prgb, int numLeds);
#define HUE_MAX_SPECTRUM 255


// hsv2rgb_raw - convert hue, saturation, and value to RGB.
//               This 'spectrum' conversion will be more green & blue
//               than a real 'rainbow', and the hue is specified just
//               in the range 0-191.  Together, these result in a
//               slightly faster conversion speed, at the expense of
//               color balance.
//
//               NOTE: Hue is 0-191 only!
//               Saturation & value are 0-255 each.
//

void hsv2rgb_raw(const struct CHSV& hsv, struct CRGB & rgb);
void hsv2rgb_raw(const struct CHSV* phsv, struct CRGB * prgb, int numLeds);
#define HUE_MAX 191


// rgb2hsv_approximate - recover _approximate_ HSV values from RGB.
//
//   NOTE 1: This function is a long-term work in process; expect
//   results to change slightly over time as this function is
//   refined and improved.
//
//   NOTE 2: This function is most accurate when the input is an
//   RGB color that came from a fully-saturated HSV color to start
//   with.  E.g. CHSV( hue, 255, 255) -> CRGB -> CHSV will give
//   best results.
//
//   NOTE 3: This function is not nearly as fast as HSV-to-RGB.
//   It is provided for those situations when the need for this
//   function cannot be avoided, or when extremely high performance
//   is not needed.
//
//   NOTE 4: Why is this 'only' an "approximation"?
//   Not all RGB colors have HSV equivalents!  For example, there
//   is no HSV value that will ever convert to RGB(255,255,0) using
//   the code provided in this library.   So if you try to
//   convert RGB(255,255,0) 'back' to HSV, you'll necessarily get
//   only an approximation.  Emphasis has been placed on getting
//   the 'hue' as close as usefully possible, but even that's a bit
//   of a challenge.  The 8-bit HSV and 8-bit RGB color spaces
//   are not a "bijection".
//
//   Nevertheless, this function does a pretty good job, particularly
//   at recovering the 'hue' from fully saturated RGB colors that
//   originally came from HSV rainbow colors.  So if you start
//   with CHSV(hue_in,255,255), and convert that to RGB, and then
//   convert it back to HSV using this function, the resulting output
//   hue will either exactly the same, or very close (+/-1).
//   The more desaturated the original RGB color is, the rougher the
//   approximation, and the less accurate the results.
//
CHSV rgb2hsv_approximate( const CRGB& rgb);

