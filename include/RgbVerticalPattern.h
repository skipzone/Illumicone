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

#include "Pattern.h"
#include "WidgetId.h"


class Widget;


class RgbVerticalPattern : public Pattern {

    public:

        RgbVerticalPattern(const std::string& name);
        ~RgbVerticalPattern();

        RgbVerticalPattern() = delete;
        RgbVerticalPattern(const RgbVerticalPattern&) = delete;
        RgbVerticalPattern& operator =(const RgbVerticalPattern&) = delete;

        bool update();       

    protected:

        bool initPattern(std::map<WidgetId, Widget*>& widgets);

    private:

        constexpr static int iRed = 0;
        constexpr static int iGreen = 1;
        constexpr static int iBlue = 2;
        constexpr static char rgbColorNames[3][] = {"red", "green", "blue"};

        std::shared_ptr<WidgetChannel> positionChannel[3];
        std::shared_ptr<WidgetChannel> greenPositionChannel;
        std::shared_ptr<WidgetChannel> bluePositionChannel;
        std::shared_ptr<WidgetChannel> widthChannel;

        int stripePos[3];
        int widthPos;

        int rNumStripes;
        int gNumStripes;
        int bNumStripes;
        int rStripeStep;
        int gStripeStep;
        int bStripeStep;

        int rScaledownFactor;
        int gScaledownFactor;
        int bScaledownFactor;
        int widthScaledownFactor;

        int maxSidebandWidth;
        int minSidebandWidth;
        int widthResetTimeoutSeconds;
        unsigned int nextResetWidthMs;
        bool resetWidth;
        int widthPosOffset;

        unsigned int pixelsPerVstring;
        unsigned int numVstrings;
        unsigned int horizontalVpixelRatio;
        unsigned int verticalVpixelRatio;
        RgbConeStrings* vpixelArray;
};

