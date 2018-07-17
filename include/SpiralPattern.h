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


class ConfigReader;
class Widget;


class SpiralPattern : public Pattern
{
    public:

        SpiralPattern(const std::string& name);
        ~SpiralPattern() {};

        SpiralPattern() = delete;
        SpiralPattern(const SpiralPattern&) = delete;
        SpiralPattern& operator =(const SpiralPattern&) = delete;

        bool update();

    protected:

        bool initPattern(ConfigReader& config, std::map<WidgetId, Widget*>& widgets);

    private:

        bool flipSpring;
        float spiralTightnessFactor;
        float progressiveSpringFactor;
        float progressiveSpringCompressionResponseFactor;

///        int compressionScaleFactor;
///        float compressionDivisor;
        int maxCyclicalCompression;
        int minCyclicalCompression;
///        float compressionFactorOffset;
        int compressionResetTimeoutSeconds;
        int compressionTriangleAmplitude;
        int compressionTrianglePeriod;

        std::shared_ptr<WidgetChannel> compressionChannel;
        std::shared_ptr<WidgetChannel> rotationChannel;
        std::shared_ptr<WidgetChannel> colorChannel;

        unsigned int nextResetCompressionMs;
        bool resetCompression;
        int compressionPos;
        int compressionPosOffset;
        float compressionFactor;
        int rotationStepIntervalMs;
        unsigned int nextRotationStepMs;
        int rotationOffset;
        bool rotateCounterclockwise;
        uint8_t currentHue;

        MeasurementMapper<int, float> compressionMeasmtMapper;
        MeasurementMapper<int, int> rotationMeasmtMapper;
        MeasurementMapper<int, int> colorMeasmtMapper;
};

