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
#include <unordered_set>

#include "IndicatorRegionsPattern.h"
#include "WidgetId.h"


class IndicatorRegion;
class Widget;


typedef struct switchState {
    bool isActivated;
    // TODO:  add last state change time
} switchState_t;


class MultiSwitchActivatedRegionsPattern : public IndicatorRegionsPattern {

    public:

        MultiSwitchActivatedRegionsPattern(const std::string& name);
        virtual ~MultiSwitchActivatedRegionsPattern() {};

        MultiSwitchActivatedRegionsPattern() = delete;
        MultiSwitchActivatedRegionsPattern(const MultiSwitchActivatedRegionsPattern&) = delete;
        MultiSwitchActivatedRegionsPattern& operator =(const MultiSwitchActivatedRegionsPattern&) = delete;

        virtual bool update();

    protected:

        virtual bool initPattern(std::map<WidgetId, Widget*>& widgets);

        std::shared_ptr<WidgetChannel> multiSwitchChannel;

    private:

        constexpr static unsigned int maxMessagesPerPatternUpdate = 30; // pulled it out of my ass

        constexpr static unsigned int maxSwitches = 16;     // steps 0 - 15

        switchState_t switchStates[maxSwitches];
        
        std::unordered_set<IndicatorRegion*> activeIndicators;
};

