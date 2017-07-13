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

// FillAndBurstPattern is based on RainbowExplosionPattern written by David Van Arnem in 2016.


#pragma once

#include <map>
#include <memory>

#include "Pattern.h"
#include "WidgetId.h"


class FillAndBurstPattern : public Pattern
{
    public:
        FillAndBurstPattern();
        ~FillAndBurstPattern() {};

        FillAndBurstPattern(const FillAndBurstPattern&) = delete;
        FillAndBurstPattern& operator =(const FillAndBurstPattern&) = delete;

        bool initPattern(ConfigReader& config, std::map<WidgetId, Widget*>& widgets, int priority);
        bool update();

    private:

        enum class PatternState {
            empty = 0,
            pressurizing = 0,
            fillRed,
            fillOrange,
            fillYellow,
            fillGreen,
            fillBlue,
            fillIndigo,
            fillViolet,
            endBursting,
            depressurizing
        };

        std::shared_ptr<WidgetChannel> pressureChannel;

        // pattern configuration
        int lowPressureCutoff;
        int burstThreshold;
        opc_pixel_t pressureColor;
        int fillStepSize;
        unsigned int fillStepIntervalMs;

        PatternState state;
        int fillPosition;
        unsigned int nextStepMs;

        void clearAllPixels();
};

