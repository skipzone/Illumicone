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

        FillAndBurstPattern(const std::string& name);
        ~FillAndBurstPattern() {};

        FillAndBurstPattern() = delete;
        FillAndBurstPattern(const FillAndBurstPattern&) = delete;
        FillAndBurstPattern& operator =(const FillAndBurstPattern&) = delete;

        bool update();

    protected:

        bool initPattern(ConfigReader& config, std::map<WidgetId, Widget*>& widgets);

    private:

        enum class PatternState {
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
        CRGB depressurizationColor;
        bool displayDepressurization;
        int fillingPriority;
        int burstingPriority;
        int lowPressureCutoff;
        int burstThreshold;
        CRGB pressurizationColor;
        int fillStepSize;
        unsigned int fillStepIntervalMs;

        PatternState state;
        int fillPosition;
        unsigned int nextStepMs;

        void clearAllPixels();
};

