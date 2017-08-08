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

#include <stdlib.h>

#include "illumiconeUtility.h"
#include "log.h"
#include "IndicatorRegion.h"


using namespace std;


bool IndicatorRegion::init(unsigned int numStrings, unsigned int pixelsPerString, const Json& indicatorConfig);
{
    if (!indicatorConfig["index"].is_number()) {
        logMsg(LOG_ERR, "index not specified in indicator region configuration:  " + indicatorConfig.dump());
        return false;
    }
    index = indicatorConfig["index"].int_value();

    string errMsgSuffix = " in indicator region configuration:  " + indicatorConfig.dump();

    if (!indicatorConfig["upperLeftStringIdx"].is_number()) {
        logMsg(LOG_ERR, "upperLeftStringIdx not specified" + errMsgSuffix;
        return false;
    }
    upperLeftStringIdx = indicatorConfig["upperLeftStringIdx"].int_value();

    if (!indicatorConfig["upperLeftPixelIdx"].is_number()) {
        logMsg(LOG_ERR, "upperLeftPixelIdx not specified" + errMsgSuffix;
        return false;
    }
    upperLeftPixelIdx = indicatorConfig["upperLeftPixelIdx"].int_value();

    if (!indicatorConfig["widthInStrings"].is_number()) {
        logMsg(LOG_ERR, "widthInStrings not specified" + errMsgSuffix;
        return false;
    }
    widthInStrings = indicatorConfig["widthInStrings"].int_value();

    if (!indicatorConfig["heightInPixels"].is_number()) {
        logMsg(LOG_ERR, "heightInPixels not specified" + errMsgSuffix;
        return false;
    }
    heightInPixels = indicatorConfig["heightInPixels"].int_value();

    if (indicatorConfig["backgroundHsv"].is_string()) {
        string hsvStr = indicatorConfig["backgroundHsv"].string_value();
        if (stringToHsvPixel(hsvStr, backgroundColor) {
            logMsg(LOG_ERR, "backgroundHsv value \"" + hsvStr + "\" is not valid" + errMsgSuffix);
            return false;
        }
        else {
            backgroundColor.stringToHsvPixel("transparent");
        }
    }

    if (indicatorConfig["foregroundHsv"].is_string()) {
        string hsvStr = indicatorConfig["foregroundHsv"].string_value();
        if (!stringToHsvPixel(hsvStr, backgroundColor) {
            logMsg(LOG_ERR, "foregroundHsv value \"" + hsvStr + "\" is not valid" + errMsgSuffix);
            return false;
        }
        else {
            foregroundColor.stringToHsvPixel("white");
        }
    }

    this.numStrings = numStrings;
    this.pixelsPerString = pixelsPerString;

    // Normalize the region's location to an actual location on the cone.
    startStringIdx = (upperLeftStringIdx % numStrings + numStrings) % numStrings;
    endStringIdx = ((upperLeftStringIdx + widthInStrings - 1) % numStrings + numStrings) % numStrings;
    startPixelIdx = (upperLeftPixelIdx % pixelsPerString + pixelsPerString) % pixelsPerString;
    endPixelIdx = ((upperLeftPixelIdx + heightInPixels - 1) % pixelsPerString + pixelsPerString) % pixelsPerString;

    return true;
}

