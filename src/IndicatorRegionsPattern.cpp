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

///#include <stdlib.h>

#include "ConfigReader.h"
//#include "illumiconeUtility.h"
#include "IndicatorRegion.h"
#include "indicatorRegionFactory.h"
#include "IndicatorRegionsPattern.h"
//#include "log.h"
#include "Widget.h"
#include "WidgetChannel.h"


using namespace std;


IndicatorRegionsPattern::IndicatorRegionsPattern(const std::string& name)
    : Pattern(name)
{
}


bool IndicatorRegionsPattern::initPattern(ConfigReader& config, std::map<WidgetId, Widget*>& widgets)
{

    // ----- get pattern configuration -----

    auto patternConfig = config.getPatternConfigJsonObject(name);


//=-=-=-= instantiate the IndicatorRegion subclass objects here

    return true;
}


bool IndicatorRegionsPattern::update()
{
    // TODO:  =-=-=-=-= call runAnimation on all the IndicatorRegion objects.  return true if any had animation.

    return isActive;
}

