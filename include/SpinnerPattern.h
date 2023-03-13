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

#include "IndicatorRegionsPattern.h"
#include "WidgetId.h"


class IndicatorRegion;
class Widget;


class SpinnerPattern : public IndicatorRegionsPattern {

    public:

        SpinnerPattern(const std::string& name);
        virtual ~SpinnerPattern() {};

        SpinnerPattern() = delete;
        SpinnerPattern(const SpinnerPattern&) = delete;
        SpinnerPattern& operator =(const SpinnerPattern&) = delete;

        virtual bool update();

    protected:

        virtual bool initPattern(std::map<WidgetId, Widget*>& widgets);

        std::shared_ptr<WidgetChannel> spinnerPositionChannel;

    private:

        IndicatorRegion* activeIndicator;
        unsigned int positionDivisor;
        unsigned int selectedBlockAnimationIntervalMs;
        unsigned int stopAnimatingMs;
};

