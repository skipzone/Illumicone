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

#include "illumiconeTypes.h"
#include "IndicatorRegionsPattern.h"
#include "WidgetId.h"


class IndicatorRegion;
class Widget;


class MidiActivatedRegionsPattern : public IndicatorRegionsPattern {

    public:

        MidiActivatedRegionsPattern(const std::string& name);
        virtual ~MidiActivatedRegionsPattern() {};

        MidiActivatedRegionsPattern() = delete;
        MidiActivatedRegionsPattern(const MidiActivatedRegionsPattern&) = delete;
        MidiActivatedRegionsPattern& operator =(const MidiActivatedRegionsPattern&) = delete;

        virtual bool update();

    protected:

        virtual bool initPattern(std::map<WidgetId, Widget*>& widgets);

        std::shared_ptr<WidgetChannel> midiInputChannel;

    private:

        constexpr static unsigned int maxMidiMessagesPerPatternUpdate = 20;  // note on and off for all 10 fingers 

        std::string midiMessageToString(MidiPositionMeasurement pos, MidiVelocityMeasurement vel);

        std::unordered_set<IndicatorRegion*> activeIndicators;
};

