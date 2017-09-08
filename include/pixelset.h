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

#include <stdlib.h>

#include "colorutils.h"
#include "pixeltypes.h"


/// Represents a set of CRGB led objects.  Provides the [] array operator, and works like a normal array in that case.
/// This should be kept in sync with the set of functions provided by CRGB as well as functions in colorutils.  Note
/// that a pixel set is a window into another set of led data, it is not its own set of led data.
template<class PIXEL_TYPE>
class CPixelView {
private:
  int8_t  dir;
  int   len;
  PIXEL_TYPE* leds;
  PIXEL_TYPE* end_pos;

public:

  /// PixelSet copy constructor
  inline CPixelView(const CPixelView & other) : dir(other.dir), len(other.len), leds(other.leds), end_pos(other.end_pos) {}

  /// pixelset constructor for a pixel set starting at the given PIXEL_TYPE* and going for _len leds.  Note that the length
  /// can be backwards, creating a PixelSet that walks backwards over the data
  /// @param leds point to the raw led data
  /// @param len how many leds in this set
  inline CPixelView(PIXEL_TYPE *_leds, int _len) : dir(_len < 0 ? -1 : 1), len(_len), leds(_leds), end_pos(_leds + _len) {}

  /// PixelSet constructor for the given set of leds, with start and end boundaries.  Note that start can be after
  /// end, resulting in a set that will iterate backwards
  /// @param leds point to the raw led data
  /// @param start the start index of the leds for this array
  /// @param end the end index of the leds for this array
  inline CPixelView(PIXEL_TYPE *_leds, int _start, int _end) : dir(((_end-_start)<0) ? -1 : 1), len((_end - _start) + dir), leds(_leds + _start), end_pos(_leds + _start + len) {}

  /// Get the size of this set
  /// @return the size of the set
  int size() const { return abs(len); }

  /// Whether or not this set goes backwards
  /// @return whether or not the set is backwards
  bool reversed() { return len < 0; }

  /// do these sets point to the same thing (note, this is different from the contents of the set being the same)
  bool operator==(const CPixelView & rhs) const { return leds == rhs.leds && len == rhs.len && dir == rhs.dir; }

  /// do these sets point to the different things (note, this is different from the contents of the set being the same)
  bool operator!=(const CPixelView & rhs) const { return leds != rhs.leds || len != rhs.len || dir != rhs.dir; }

  /// access a single element in this set, just like an array operator
  inline PIXEL_TYPE & operator[](int x) const { if(dir & 0x80) { return leds[-x]; } else { return leds[x]; } }

  /// Access an inclusive subset of the leds in this set.  Note that start can be greater than end, which will
  /// result in a reverse ordering for many functions (useful for mirroring)
  /// @param start the first element from this set for the new subset
  /// @param end the last element for the new subset
  inline CPixelView operator()(int start, int end) { return CPixelView(leds, start, end); }

  /// Access an inclusive subset of the leds in this set, starting from the first.
  /// @param end the last element for the new subset
  /// Not sure i want this? inline CPixelView operator()(int end) { return CPixelView(leds, 0, end); }

  /// Return the reverse ordering of this set
  inline CPixelView operator-() { return CPixelView(leds + len - dir, len - dir, 0); }

  /// Return a pointer to the first element in this set
  inline operator PIXEL_TYPE* () const { return leds; }

  /// Assign the passed in color to all elements in this set
  /// @param color the new color for the elements in the set
  inline CPixelView & operator=(const PIXEL_TYPE & color) {
    for(iterator pixel = begin(), _end = end(); pixel != _end; ++pixel) { (*pixel) = color; }
    return *this;
  }


  void dump() const {
/**
    Serial.print("len: "); Serial.print(len); Serial.print(", dir:"); Serial.print((int)dir);
    Serial.print(", range:"); Serial.print((uint32_t)leds); Serial.print("-"); Serial.print((uint32_t)end_pos);
    Serial.print(", diff:"); Serial.print((int32_t)(end_pos - leds));
    Serial.println("");
 **/
 }

  /// Copy the contents of the passed in set to our set.  Note if one set is smaller than the other, only the
  /// smallest number of items will be copied over.
  inline CPixelView & operator=(const CPixelView & rhs) {
    for(iterator pixel = begin(), rhspixel = rhs.begin(), _end = end(), rhs_end = rhs.end(); (pixel != _end) && (rhspixel != rhs_end); ++pixel, ++rhspixel) {
      (*pixel) = (*rhspixel);
    }
    return *this;
  }

  /// Facilitate resizing or replacing the pixel array.
  inline CPixelView& resize(PIXEL_TYPE *_leds, int _start, int _end)
  {
    // TODO 7/27/2017 ross:  untested
    dir = (_end - _start) < 0 ? -1 : 1;
    len = (_end - _start) + dir;
    leds = _leds + _start;
    end_pos = _leds + _start + len;
    return *this;
  }
  inline CPixelView& resize(PIXEL_TYPE *_leds, int _len)
  {
    dir = _len < 0 ? -1 : 1;
    len = _len;
    leds = _leds;
    end_pos = _leds + _len;
    return *this;
  }

  /// @name modification/scaling operators
  //@{
  /// Add the passed in value to r,g, b for all the pixels in this set
  inline CPixelView & addToRGB(uint8_t inc) { for(iterator pixel = begin(), _end = end(); pixel != _end; ++pixel) { (*pixel) += inc; } return *this; }
  /// Add every pixel in the other set to this set
  inline CPixelView & operator+=(CPixelView & rhs) { for(iterator pixel = begin(), rhspixel = rhs.begin(), _end = end(), rhs_end = rhs.end(); (pixel != _end) && (rhspixel != rhs_end); ++pixel, ++rhspixel) { (*pixel) += (*rhspixel); } return *this; }

  /// Subtract the passed in value from r,g,b for all pixels in this set
  inline CPixelView & subFromRGB(uint8_t inc) { for(iterator pixel = begin(), _end = end(); pixel != _end; ++pixel) { (*pixel) -= inc; } return *this; }
  /// Subtract every pixel in the other set from this set
  inline CPixelView & operator-=(CPixelView & rhs) { for(iterator pixel = begin(), rhspixel = rhs.begin(), _end = end(), rhs_end = rhs.end(); (pixel != _end) && (rhspixel != rhs_end); ++pixel, ++rhspixel) { (*pixel) -= (*rhspixel); } return *this; }

  /// Increment every pixel value in this set
  inline CPixelView & operator++() { for(iterator pixel = begin(), _end = end(); pixel != _end; ++pixel) { (*pixel)++; } return *this; }
  /// Increment every pixel value in this set
  inline CPixelView & operator++(int DUMMY_ARG) { for(iterator pixel = begin(), _end = end(); pixel != _end; ++pixel) { (*pixel)++; } return *this; }

  /// Decrement every pixel value in this set
  inline CPixelView & operator--() { for(iterator pixel = begin(), _end = end(); pixel != _end; ++pixel) { (*pixel)--; } return *this; }
  /// Decrement every pixel value in this set
  inline CPixelView & operator--(int DUMMY_ARG) { for(iterator pixel = begin(), _end = end(); pixel != _end; ++pixel) { (*pixel)--; } return *this; }

  /// Divide every led by the given value
  inline CPixelView & operator/=(uint8_t d) { for(iterator pixel = begin(), _end = end(); pixel != _end; ++pixel) { (*pixel) /= d; } return *this; }
  /// Shift every led in this set right by the given number of bits
  inline CPixelView & operator>>=(uint8_t d) { for(iterator pixel = begin(), _end = end(); pixel != _end; ++pixel) { (*pixel) >>= d; } return *this; }
  /// Multiply every led in this set by the given value
  inline CPixelView & operator*=(uint8_t d) { for(iterator pixel = begin(), _end = end(); pixel != _end; ++pixel) { (*pixel) *= d; } return *this; }

  /// Scale every led by the given scale
  inline CPixelView & nscale8_video(uint8_t scaledown) { for(iterator pixel = begin(), _end = end(); pixel != _end; ++pixel) { (*pixel).nscale8_video(scaledown); } return *this;}
  /// Scale down every led by the given scale
  inline CPixelView & operator%=(uint8_t scaledown) { for(iterator pixel = begin(), _end = end(); pixel != _end; ++pixel) { (*pixel).nscale8_video(scaledown); } return *this; }
  /// Fade every led down by the given scale
  inline CPixelView & fadeLightBy(uint8_t fadefactor) { return nscale8_video(255 - fadefactor); }

  /// Scale every led by the given scale
  inline CPixelView & nscale8(uint8_t scaledown) { for(iterator pixel = begin(), _end = end(); pixel != _end; ++pixel) { (*pixel).nscale8(scaledown); } return *this; }
  /// Scale every led by the given scale
  inline CPixelView & nscale8(PIXEL_TYPE & scaledown) { for(iterator pixel = begin(), _end = end(); pixel != _end; ++pixel) { (*pixel).nscale8(scaledown); } return *this; }
  /// Scale every led in this set by every led in the other set
  inline CPixelView & nscale8(CPixelView & rhs) { for(iterator pixel = begin(), rhspixel = rhs.begin(), _end = end(), rhs_end = rhs.end(); (pixel != _end) && (rhspixel != rhs_end); ++pixel, ++rhspixel) { (*pixel).nscale8((*rhspixel)); } return *this; }

  /// Fade every led down by the given scale
  inline CPixelView & fadeToBlackBy(uint8_t fade) { return nscale8(255 - fade); }

  /// Apply the PIXEL_TYPE |= operator to every pixel in this set with the given PIXEL_TYPE value (bringing each channel to the higher of the two values)
  inline CPixelView & operator|=(const PIXEL_TYPE & rhs) { for(iterator pixel = begin(), _end = end(); pixel != _end; ++pixel) { (*pixel) |= rhs; } return *this; }
  /// Apply the PIXEL_TYPE |= operator to every pixel in this set with every pixel in the passed in set
  inline CPixelView & operator|=(const CPixelView & rhs) { for(iterator pixel = begin(), rhspixel = rhs.begin(), _end = end(), rhs_end = rhs.end(); (pixel != _end) && (rhspixel != rhs_end); ++pixel, ++rhspixel) { (*pixel) |= (*rhspixel); } return *this; }
  /// Apply the PIXEL_TYPE |= operator to every pixel in this set
  inline CPixelView & operator|=(uint8_t d) { for(iterator pixel = begin(), _end = end(); pixel != _end; ++pixel) { (*pixel) |= d; } return *this; }

  /// Apply the PIXEL_TYPE &= operator to every pixel in this set with the given PIXEL_TYPE value (bringing each channel down to the lower of the two values)
  inline CPixelView & operator&=(const PIXEL_TYPE & rhs) { for(iterator pixel = begin(), _end = end(); pixel != _end; ++pixel) { (*pixel) &= rhs; } return *this; }
  /// Apply the PIXEL_TYPE &= operator to every pixel in this set with every pixel in the passed in set
  inline CPixelView & operator&=(const CPixelView & rhs) { for(iterator pixel = begin(), rhspixel = rhs.begin(), _end = end(), rhs_end = rhs.end(); (pixel != _end) && (rhspixel != rhs_end); ++pixel, ++rhspixel) { (*pixel) &= (*rhspixel); } return *this; }
  /// APply the PIXEL_TYPE &= operator to every pixel in this set with the passed in value
  inline CPixelView & operator&=(uint8_t d) { for(iterator pixel = begin(), _end = end(); pixel != _end; ++pixel) { (*pixel) &= d; } return *this; }
  //@}

  /// Returns whether or not any leds in this set are non-zero
  inline operator bool() { for(iterator pixel = begin(), _end = end(); pixel != _end; ++pixel) { if((*pixel)) return true; } return false; }

  // Color util functions
  inline CPixelView & fill_solid(const PIXEL_TYPE & color) { *this = color; return *this; }
  /// what the hell is this?  inline CPixelView & fill_solid(const CHSV & color) { if(dir>0) { *this = color; return *this; } }

  inline CPixelView & fill_rainbow(uint8_t initialhue, uint8_t deltahue=5) {
    if(dir >= 0) {
      ::fill_rainbow(leds,len,initialhue,deltahue);
    } else {
      ::fill_rainbow(leds+len+1,-len,initialhue,deltahue);
    }
    return *this;
  }

  inline CPixelView & fill_gradient(const CHSV & startcolor, const CHSV & endcolor, TGradientDirectionCode directionCode  = SHORTEST_HUES) {
    if(dir >= 0) {
      ::fill_gradient(leds,len,startcolor, endcolor, directionCode);
    } else {
      ::fill_gradient(leds + len + 1, (-len), endcolor, startcolor, directionCode);
    }
    return *this;
  }

  inline CPixelView & fill_gradient(const CHSV & c1, const CHSV & c2, const CHSV &  c3, TGradientDirectionCode directionCode = SHORTEST_HUES) {
    if(dir >= 0) {
      ::fill_gradient(leds, len, c1, c2, c3, directionCode);
    } else {
      ::fill_gradient(leds + len + 1, -len, c3, c2, c1, directionCode);
    }
    return *this;
  }

  inline CPixelView & fill_gradient(const CHSV & c1, const CHSV & c2, const CHSV & c3, const CHSV & c4, TGradientDirectionCode directionCode = SHORTEST_HUES) {
    if(dir >= 0) {
      ::fill_gradient(leds, len, c1, c2, c3, c4, directionCode);
    } else {
      ::fill_gradient(leds + len + 1, -len, c4, c3, c2, c1, directionCode);
    }
    return *this;
  }

  inline CPixelView & fill_gradient_RGB(const PIXEL_TYPE & startcolor, const PIXEL_TYPE & endcolor, TGradientDirectionCode directionCode  = SHORTEST_HUES) {
    if(dir >= 0) {
      ::fill_gradient_RGB(leds,len,startcolor, endcolor);
    } else {
      ::fill_gradient_RGB(leds + len + 1, (-len), endcolor, startcolor);
    }
    return *this;
  }

  inline CPixelView & fill_gradient_RGB(const PIXEL_TYPE & c1, const PIXEL_TYPE & c2, const PIXEL_TYPE &  c3) {
    if(dir >= 0) {
      ::fill_gradient_RGB(leds, len, c1, c2, c3);
    } else {
      ::fill_gradient_RGB(leds + len + 1, -len, c3, c2, c1);
    }
    return *this;
  }

  inline CPixelView & fill_gradient_RGB(const PIXEL_TYPE & c1, const PIXEL_TYPE & c2, const PIXEL_TYPE & c3, const PIXEL_TYPE & c4) {
    if(dir >= 0) {
      ::fill_gradient_RGB(leds, len, c1, c2, c3, c4);
    } else {
      ::fill_gradient_RGB(leds + len + 1, -len, c4, c3, c2, c1);
    }
    return *this;
  }

  inline CPixelView & nblend(const PIXEL_TYPE & overlay, fract8 amountOfOverlay) { for(iterator pixel = begin(), _end = end(); pixel != _end; ++pixel) { ::nblend((*pixel), overlay, amountOfOverlay); } return *this; }
  inline CPixelView & nblend(const CPixelView & rhs, fract8 amountOfOverlay) { for(iterator pixel = begin(), rhspixel = rhs.begin(), _end = end(), rhs_end = rhs.end(); (pixel != _end) && (rhspixel != rhs_end); ++pixel, ++rhspixel) { ::nblend((*pixel), (*rhspixel), amountOfOverlay); } return *this; }

  // Note: only bringing in a 1d blur, not sure 2d blur makes sense when looking at sub arrays
  inline CPixelView & blur1d(fract8 blur_amount) {
    if(dir >= 0) {
      ::blur1d(leds, len, blur_amount);
    } else {
      ::blur1d(leds + len + 1, -len, blur_amount);
    }
    return *this;
  }

  inline CPixelView & napplyGamma_video(float gamma) {
    if(dir >= 0) {
      ::napplyGamma_video(leds, len, gamma);
    } else {
      ::napplyGamma_video(leds + len + 1, -len, gamma);
    }
    return *this;
  }

  inline CPixelView & napplyGamma_video(float gammaR, float gammaG, float gammaB) {
    if(dir >= 0) {
      ::napplyGamma_video(leds, len, gammaR, gammaG, gammaB);
    } else {
      ::napplyGamma_video(leds + len + 1, -len, gammaR, gammaG, gammaB);
    }
    return *this;
  }

  // TODO: Make this a fully specified/proper iterator
  template <class T>
  class pixelset_iterator_base {
    T * leds;
    const int8_t dir;
  public:
    __attribute__((always_inline)) inline pixelset_iterator_base(const pixelset_iterator_base & rhs) : leds(rhs.leds), dir(rhs.dir) {}
    __attribute__((always_inline)) inline pixelset_iterator_base(T * _leds, const char _dir) : leds(_leds), dir(_dir) {}

    __attribute__((always_inline)) inline pixelset_iterator_base& operator++() { leds += dir; return *this; }
    __attribute__((always_inline)) inline pixelset_iterator_base operator++(int) { pixelset_iterator_base tmp(*this); leds += dir; return tmp; }

    __attribute__((always_inline)) inline bool operator==(pixelset_iterator_base & other) const { return leds == other.leds; } // && set==other.set; }
    __attribute__((always_inline)) inline bool operator!=(pixelset_iterator_base & other) const { return leds != other.leds; } // || set != other.set; }

    __attribute__((always_inline)) inline PIXEL_TYPE& operator*() const { return *leds; }
  };

  typedef pixelset_iterator_base<PIXEL_TYPE> iterator;
  typedef pixelset_iterator_base<const PIXEL_TYPE> const_iterator;

  iterator begin() { return iterator(leds, dir); }
  iterator end() { return iterator(end_pos, dir); }

  iterator begin() const { return iterator(leds, dir); }
  iterator end() const { return iterator(end_pos, dir); }

  const_iterator cbegin() const { return const_iterator(leds, dir); }
  const_iterator cend() const { return const_iterator(end_pos, dir); }
};

typedef CPixelView<CRGB> CRGBSet;

__attribute__((always_inline))
inline CRGB *operator+(const CRGBSet & pixels, int offset) { return (CRGB*)pixels + offset; }


template<int SIZE>
class CRGBArray : public CPixelView<CRGB> {
  CRGB rawleds[SIZE];
public:
  CRGBArray() : CPixelView<CRGB>(rawleds, SIZE) {}
};


template<int SIZE>
class CHSVArray : public CPixelView<CHSV> {
  CHSV rawleds[SIZE];
public:
  CHSVArray() : CPixelView<CHSV>(rawleds, SIZE) {}
};

