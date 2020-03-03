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

#include <map>
#include <memory>

#include "MeasurementMapper.h"
#include "Pattern.h"
#include "WidgetId.h"


class Widget;


class StripePattern : public Pattern {

    public:

        StripePattern(const std::string& name);
        ~StripePattern() {};

        StripePattern() = delete;
        StripePattern(const StripePattern&) = delete;
        StripePattern& operator =(const StripePattern&) = delete;

        bool update();       

    protected:

        bool initPattern(std::map<WidgetId, Widget*>& widgets);

    private:

        std::shared_ptr<WidgetChannel> positionChannel;
        std::shared_ptr<WidgetChannel> widthChannel;
        std::shared_ptr<WidgetChannel> hueChannel;
        std::shared_ptr<WidgetChannel> saturationChannel;

        MeasurementMapper<int, float> hueMeasmtMapper;
        MeasurementMapper<int, float> saturationMeasmtMapper;
        MeasurementMapper<int, int> positionMeasmtMapper;
        MeasurementMapper<int, int> widthMeasmtMapper;

        int stripeVirtualPos;
        int widthPos;

        int numStripes;
        int stripeStep;

        int scaledownFactor;
        int widthScaledownFactor;

        int maxSidebandWidth;
        int minSidebandWidth;

        HsvPixel stripeHsv;

        float startingHue;
        float endingHue;
        bool hueDirectionIsBlueToRed;
        float hueFoldbackPct;
        unsigned int hueRepeat;
        float startingSaturation;
        float endingSaturation;
        bool saturationDirectionIsDecreasing;
        float saturationFoldbackPct;
        unsigned int saturationRepeat;

        int widthResetTimeoutSeconds;
        unsigned int nextResetWidthMs;
        bool resetWidth;
        int widthPosOffset;

        // isHorizontal refers to the orientation of the line(s).  When true, we will
        // draw horizontal lines (rings) that move vertically along the strings.  When
        // false, we will draw vertical lines that move horizontally around the cone.
        bool isHorizontal;

        unsigned int virtualPixelRatio;

        // The drawing planes are perpenticular to the stripe orientation.  For a vertical
        // stripe, a horizontal drawing plane exists at each string pixel.  The number of
        // pixels in the plane is equal to the number of strings.  The stripe is drawn by
        // illumiating one or more pixels at the same relative positions in each
        // plane.  Which pixels are illuminated is determined by the stripe's position and
        // width.  The stripe moves horizontally across those parallel, horizontal planes.
        // Similarly, for a horizontal stripe, a vertical drawing plane exists at each string.
        unsigned int numPixelsInDrawingPlane;
        unsigned int numVirtualPixelsInDrawingPlane;

};

