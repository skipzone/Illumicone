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


class SparklePattern : public Pattern {

    public:

        SparklePattern(const std::string& name);
        ~SparklePattern() {};

        SparklePattern() = delete;
        SparklePattern(const SparklePattern&) = delete;
        SparklePattern& operator =(const SparklePattern&) = delete;

        bool update();

    protected:

        bool initPattern(std::map<WidgetId, Widget*>& widgets);

    private:

        void setIsActive(bool nowActive, unsigned int nowMs);

        std::shared_ptr<WidgetChannel> densityChannel;

        // pattern configuration
        int densityScaledownFactor;
        bool doAutoDecay;
        float decayConstant;
        unsigned int decayResetMs;
        int activationThreshold;
        int deactivationThreshold;
        CRGB forwardSparkleColor;
        unsigned int numGoodMeasurementsForReactivation;
        CRGB reverseSparkleColor;
        bool usePositionMeasurement;
        bool useRandomColors;

        unsigned int goodMeasurementCount;
        bool motionIsReverse;
        unsigned int nextSparkleChangeMs;
        int numPixelsPerStringToSparkle;
        unsigned int sparkleChangeIntervalMs;
        unsigned int decayStartMs;
        unsigned int inactiveStartMs;
};

