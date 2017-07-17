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

#ifndef EYE_WIDGET_H
#define EYE_WIDGET_H

#include "Widget.h"
#include "WidgetChannel.h"

class EyeWidget : public Widget
{
    public:
        EyeWidget();
        ~EyeWidget() {};

        EyeWidget(const EyeWidget&) = delete;
        EyeWidget& operator = (const EyeWidget&) = delete;

        bool moveData();

    private:

        // for generating simulated measurements
        constexpr static unsigned int updateIntervalMs = 100;
        constexpr static unsigned int numActiveSteps = 30;
        constexpr static unsigned int numInactiveSteps = 50;
        constexpr static int activePositionValue = 201;
        unsigned int stepCount;
        unsigned int lastUpdateMs;
};

#endif /* EYE_WIDGET_H */
