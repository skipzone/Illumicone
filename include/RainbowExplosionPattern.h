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


class RainbowExplosionPattern : public Pattern
{
    public:

        RainbowExplosionPattern(const std::string& name);
        ~RainbowExplosionPattern() {};

        RainbowExplosionPattern() = delete;
        RainbowExplosionPattern(const RainbowExplosionPattern&) = delete;
        RainbowExplosionPattern& operator =(const RainbowExplosionPattern&) = delete;

        bool update();

    protected:

        bool initPattern(ConfigReader& config, std::map<WidgetId, Widget*>& widgets);

    private:

        enum class PatternState {
            fizzle = 0,
            fillRed,
            fillOrange,
            fillYellow,
            fillGreen,
            fillBlue,
            fillIndigo,
            fillViolet,
            endExplosion
        };

        std::shared_ptr<WidgetChannel> intensityChannel;

        // pattern configuration
        int activationThreshold;
        int explosionThreshold;
        int accumulatorResetUpperLimit;
        int minFizzleFill;
        int maxFizzleFill;
        int fillStepSize;
        unsigned int fillStepIntervalMs;
        constexpr static unsigned int fizzleMeasurementTimeoutPeriodMs = 1000;

        PatternState state;
        int fillPosition;
        int accumulator;
        unsigned int nextStepMs;
        unsigned int fizzleMeasurementTimeoutMs;

        void clearAllPixels();
};

