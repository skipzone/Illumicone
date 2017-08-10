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

#include "IndicatorRegion.h"


class SimpleBlockIndicator : public IndicatorRegion {

    public:

        SimpleBlockIndicator() {};
        virtual ~SimpleBlockIndicator() {};

        SimpleBlockIndicator(const SimpleBlockIndicator&) = delete;
        SimpleBlockIndicator& operator =(const SimpleBlockIndicator&) = delete;

        bool init(unsigned int numStrings, unsigned int pixelsPerString, const json11::Json& indicatorConfig);
        void makeAnimating(bool enable);
        bool runAnimation();
        void transitionOff();
        void transitionOn();
        void turnOffImmediately();
        void turnOnImmediately();

    protected:

    private:

        enum class AnimationState {
            inactive = 0,
            transitionOnStart,
            transitionOn,
            transitionOffStart,
            transitionOff,
            flashStart,
            flashOn,
            flashOnWait,
            flashOff,
            flashOffWait
        };

        // configuration
        unsigned int fadeIntervalMs;
        unsigned int flashIntervalMs;

        AnimationState state;
        unsigned int fadeStepMs;
        float fadeStepValue;
        float fadeValue;
        bool isOn;
        unsigned int nextFadeChangeMs;
        unsigned int nextFlashChangeMs;
};


