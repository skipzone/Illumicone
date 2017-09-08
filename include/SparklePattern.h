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


class ConfigReader;
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

        bool initPattern(ConfigReader& config, std::map<WidgetId, Widget*>& widgets);

    private:

        std::shared_ptr<WidgetChannel> densityChannel;

        // pattern configuration
        int densityScaledownFactor;
        int activationThreshold;
        int deactivationThreshold;
        unsigned int numGoodMeasurementsForReactivation;
        CRGB sparkleColor;
        bool usePositionMeasurement;
        bool useRandomColors;

        unsigned int goodMeasurementCount;
        unsigned int nextSparkleChangeMs;
        int numPixelsPerStringToSparkle;
        unsigned int sparkleChangeIntervalMs;
};

