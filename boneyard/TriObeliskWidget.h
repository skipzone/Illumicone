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

#include "Widget.h"
#include "WidgetChannel.h"

class TriObeliskWidget : public Widget
{
    public:

        TriObeliskWidget();
        virtual ~TriObeliskWidget() {};

        TriObeliskWidget(const TriObeliskWidget&) = delete;
        TriObeliskWidget& operator =(const TriObeliskWidget&) = delete;

        void updateChannelSimulatedMeasurements(unsigned int chIdx);

    private:

        // TODO 8/7/2017 ross:  These need to be sized dynamically to agree with the number of channels.
        bool simulationIsActive[8];
        unsigned int simulationToggleActivityMs[8];
        unsigned int simulationToggleActivityPeriodMs[8];
        int minPosition[8];
        int maxPosition[8];
};

