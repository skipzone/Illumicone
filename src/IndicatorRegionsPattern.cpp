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
#include "log.h"
#include "Widget.h"
#include "WidgetChannel.h"


using namespace std;


IndicatorRegionsPattern::IndicatorRegionsPattern(const std::string& name, bool usesHsvModel)
    : Pattern(name, usesHsvModel)
{
}


IndicatorRegionsPattern::~IndicatorRegionsPattern()
{
    for (auto&& indicatorRegion : indicatorRegions) {
        delete indicatorRegion;
    }
}


bool IndicatorRegionsPattern::initPattern(ConfigReader& config, std::map<WidgetId, Widget*>& widgets)
{
    // ----- get pattern configuration -----

    auto patternConfig = config.getPatternConfigJsonObject(name);

    string errMsgSuffix = " in " + name + " pattern configuration.";

    // indicatorClassName is optional at the pattern level.  If it is not specified,
    // for the pattern, it msut be specified in each indicator's configuration.
    string indicatorClassName = patternConfig["indicatorClassName"].string_value();

    unsigned int numberOfIndicators;
    if (!ConfigReader::getUnsignedIntValue(patternConfig, "numberOfIndicators", numberOfIndicators, errMsgSuffix, 0)) {
        return false;
    }

    if (!patternConfig["indicators"].is_array()) {
        logMsg(LOG_ERR, "indicators is not present or is not an array" + errMsgSuffix);
        return false;
    }
    auto indicatorConfigs = patternConfig["indicators"].array_items();
    if (indicatorConfigs.size() != numberOfIndicators) {
        logMsg(LOG_ERR, "numberOfIndicators is " + to_string(numberOfIndicators)
                        + ", but there are " + to_string(indicatorConfigs.size())
                        + " indicators configured" + errMsgSuffix);
        return false;
    }


    // ----- instantiate the IndicatorRegion subclass objects -----

    indicatorRegions.resize(numberOfIndicators, nullptr);
    
    for (auto& indicatorConfig : indicatorConfigs) {

        string thisIndicatorClassName = indicatorConfig["indicatorClassName"].string_value();
        if (thisIndicatorClassName.empty()) {
            thisIndicatorClassName = indicatorClassName;
        }
        if (thisIndicatorClassName.empty()) {
            logMsg(LOG_ERR, "indicatorClassName was not specified at pattern level"
                            " and is not present or is empty in an indicator configuration"
                            + errMsgSuffix);
            return false;
        }

        IndicatorRegion* newIndicatorRegion = indicatorRegionFactory(thisIndicatorClassName);
        if (newIndicatorRegion == nullptr) {
            logMsg(LOG_ERR, "Unable to instantiate IndicatorRegion object" + errMsgSuffix);
            return false;
        }
        if (!newIndicatorRegion->init(numStrings, pixelsPerString, indicatorConfig)) {
            logMsg(LOG_ERR, "Initialization of IndicatorRegion object failed" + errMsgSuffix);
            return false;
        }

        unsigned int indicatorRegionIndex = newIndicatorRegion->getIndex();
        if (indicatorRegionIndex >= numberOfIndicators) {
            logMsg(LOG_ERR, "Indicator index " + to_string(indicatorRegionIndex)
                            + " is out of bounds" + errMsgSuffix);
            return false;
        }

        if (indicatorRegions[indicatorRegionIndex] != nullptr) {
            logMsg(LOG_ERR, "There are multiple configurations for the indicator with index "
                            + to_string(indicatorRegionIndex) + errMsgSuffix);
            return false;
        }

        newIndicatorRegion->setConeStrings(&coneStrings);

        indicatorRegions[indicatorRegionIndex] = newIndicatorRegion;
    }

    logMsg(LOG_INFO, name + " instantiated " + to_string(numberOfIndicators) + " indicator regions.");

    return true;
}


bool IndicatorRegionsPattern::update()
{
    bool anyAnimationIsActive = false;
    for (auto&& indicatorRegion : indicatorRegions) {
        if (indicatorRegion->runAnimation()) {
            anyAnimationIsActive = true;
        }
    }
    return anyAnimationIsActive;
}

