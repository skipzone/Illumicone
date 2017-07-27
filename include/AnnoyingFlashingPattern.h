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

#include "pixelset.h"
#include <vector>


class ConfigReader;
class Widget;


class AnnoyingFlashingPattern : public Pattern {

    public:

        AnnoyingFlashingPattern(const std::string& name);
        ~AnnoyingFlashingPattern();

        AnnoyingFlashingPattern() = delete;
        AnnoyingFlashingPattern(const AnnoyingFlashingPattern&) = delete;
        AnnoyingFlashingPattern& operator =(const AnnoyingFlashingPattern&) = delete;

        bool update();

    protected:

        bool initPattern(ConfigReader& config, std::map<WidgetId, Widget*>& widgets);

    private:

        std::shared_ptr<WidgetChannel> intensityChannel;

        int activationThreshold;
        time_t flashingTimeoutSeconds;
        time_t timeExceededThreshold;

        std::vector<CPixelView<CHSV>*> hsvPixelStrings;

        void goInactive();
};

