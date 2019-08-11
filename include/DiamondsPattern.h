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


class DiamondsPattern : public Pattern {

    public:

        DiamondsPattern(const std::string& name);
        ~DiamondsPattern() {};

        DiamondsPattern() = delete;
        DiamondsPattern(const DiamondsPattern&) = delete;
        DiamondsPattern& operator =(const DiamondsPattern&) = delete;

        bool update();       

    protected:

        bool initPattern(std::map<WidgetId, Widget*>& widgets);

    private:

        static constexpr int numOrientations = 2;
        static constexpr char orientationPrefix[numOrientations + 1] = "hv";
        static constexpr int horiz = 0;
        static constexpr int vert = 1;

        static constexpr int numColors = 3;
        static constexpr char rgbPrefix[numColors + 1] = "rgb";

        bool orientationIsEnabled[numOrientations];

        std::shared_ptr<WidgetChannel> positionChannel[numColors];
        std::shared_ptr<WidgetChannel> widthChannel;

        int stripeVPos[numOrientations][numColors];
        int widthPos[numOrientations][numColors];

        int numStripes[numOrientations][numColors];
        int stripeStep[numOrientations][numColors];

        int scaledownFactor[numOrientations][numColors];
        int widthScaledownFactor[numOrientations];

        int maxSidebandWidth[numOrientations][numColors];
        int minSidebandWidth[numOrientations][numColors];

        uint8_t baseIntensity[numOrientations][numColors];

        int widthResetTimeoutSeconds;
        unsigned int nextResetWidthMs;
        bool resetWidth;
        int widthPosOffset;

        unsigned int vPixelsPerString;
        unsigned int numVStrings;
        unsigned int horizontalVPixelRatio;
        unsigned int verticalVPixelRatio;
        unsigned int numPixelsForOrientation[numOrientations];
        unsigned int numVPixelsForOrientation[numOrientations];
};

