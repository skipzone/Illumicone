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


/// contains definitions for color correction and temperature

/// definitions for color correction and light temperatures

typedef enum {
   // Color correction starting points

   /// typical values for SMD5050 LEDs
   ///@{
    TypicalSMD5050=0xFFB0F0 /* 255, 176, 240 */,
    TypicalLEDStrip=0xFFB0F0 /* 255, 176, 240 */,
  ///@}

   /// typical values for 8mm "pixels on a string"
   /// also for many through-hole 'T' package LEDs
   ///@{
   Typical8mmPixel=0xFFE08C /* 255, 224, 140 */,
   TypicalPixelString=0xFFE08C /* 255, 224, 140 */,
   ///@}

   /// uncorrected color
   UncorrectedColor=0xFFFFFF

} LEDColorCorrection;


typedef enum {
   /// @name Black-body radiation light sources
   /// Black-body radiation light sources emit a (relatively) continuous
   /// spectrum, and can be described as having a Kelvin 'temperature'
   ///@{
   /// 1900 Kelvin
   Candle=0xFF9329 /* 1900 K, 255, 147, 41 */,
   /// 2600 Kelvin
   Tungsten40W=0xFFC58F /* 2600 K, 255, 197, 143 */,
   /// 2850 Kelvin
   Tungsten100W=0xFFD6AA /* 2850 K, 255, 214, 170 */,
   /// 3200 Kelvin
   Halogen=0xFFF1E0 /* 3200 K, 255, 241, 224 */,
   /// 5200 Kelvin
   CarbonArc=0xFFFAF4 /* 5200 K, 255, 250, 244 */,
   /// 5400 Kelvin
   HighNoonSun=0xFFFFFB /* 5400 K, 255, 255, 251 */,
   /// 6000 Kelvin
   DirectSunlight=0xFFFFFF /* 6000 K, 255, 255, 255 */,
   /// 7000 Kelvin
   OvercastSky=0xC9E2FF /* 7000 K, 201, 226, 255 */,
   /// 20000 Kelvin
   ClearBlueSky=0x409CFF /* 20000 K, 64, 156, 255 */,
   ///@}

   /// @name Gaseous light sources
   /// Gaseous light sources emit discrete spectral bands, and while we can
   /// approximate their aggregate hue with RGB values, they don't actually
   /// have a proper Kelvin temperature.
   ///@{
   WarmFluorescent=0xFFF4E5 /* 0 K, 255, 244, 229 */,
   StandardFluorescent=0xF4FFFA /* 0 K, 244, 255, 250 */,
   CoolWhiteFluorescent=0xD4EBFF /* 0 K, 212, 235, 255 */,
   FullSpectrumFluorescent=0xFFF4F2 /* 0 K, 255, 244, 242 */,
   GrowLightFluorescent=0xFFEFF7 /* 0 K, 255, 239, 247 */,
   BlackLightFluorescent=0xA700FF /* 0 K, 167, 0, 255 */,
   MercuryVapor=0xD8F7FF /* 0 K, 216, 247, 255 */,
   SodiumVapor=0xFFD1B2 /* 0 K, 255, 209, 178 */,
   MetalHalide=0xF2FCFF /* 0 K, 242, 252, 255 */,
   HighPressureSodium=0xFFB74C /* 0 K, 255, 183, 76 */,
   ///@}

   /// Uncorrected temperature 0xFFFFFF
   UncorrectedTemperature=0xFFFFFF
} ColorTemperature;

