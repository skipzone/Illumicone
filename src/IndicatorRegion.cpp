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
#include <string>

#include "ConfigReader.h"
#include "illumiconeUtility.h"
#include "illumiconePixelUtility.h"
#include "Log.h"
#include "IndicatorRegion.h"

using namespace std;


extern Log logger;


IndicatorRegion::IndicatorRegion()
    : isAnimating(false)
    , isHighlighted(false)
    , isOn(false)
    , isSubtle(false)
    , isTransitioning(false)
{
}


bool IndicatorRegion::init(unsigned int numStrings, unsigned int pixelsPerString, const json11::Json& indicatorConfigObject)
{
    string errMsgSuffix = " in indicator region configuration:  " + indicatorConfigObject.dump();

    if (!ConfigReader::getUnsignedIntValue(indicatorConfigObject, "index", index, errMsgSuffix)) {
        return false;
    }

    if (!ConfigReader::getUnsignedIntValue(indicatorConfigObject, "upperLeftStringIdx", upperLeftStringIdx, errMsgSuffix)) {
        return false;
    }

    if (!ConfigReader::getUnsignedIntValue(indicatorConfigObject, "upperLeftPixelIdx", upperLeftPixelIdx, errMsgSuffix)) {
        return false;
    }

    if (!ConfigReader::getUnsignedIntValue(indicatorConfigObject, "widthInStrings", widthInStrings, errMsgSuffix)) {
        return false;
    }

    if (!ConfigReader::getUnsignedIntValue(indicatorConfigObject, "heightInPixels", heightInPixels, errMsgSuffix)) {
        return false;
    }

    if (indicatorConfigObject["backgroundHsv"].is_string()) {
        string hsvStr = indicatorConfigObject["backgroundHsv"].string_value();
        if (stringToHsvPixel(hsvStr, backgroundColor)) {
            logger.logMsg(LOG_ERR, "backgroundHsv value \"" + hsvStr + "\" is not valid" + errMsgSuffix);
            return false;
        }
    }
    else {
        stringToHsvPixel("transparent", backgroundColor);
    }

    if (indicatorConfigObject["foregroundHsv"].is_string()) {
        string hsvStr = indicatorConfigObject["foregroundHsv"].string_value();
        if (!stringToHsvPixel(hsvStr, foregroundColor)) {
            logger.logMsg(LOG_ERR, "foregroundHsv value \"" + hsvStr + "\" is not valid" + errMsgSuffix);
            return false;
        }
    }
    else {
        stringToHsvPixel("white", foregroundColor);
    }

    this->numStrings = numStrings;
    this->pixelsPerString = pixelsPerString;

    // Normalize the region's location to an actual location on the cone.
    startStringIdx = (upperLeftStringIdx % numStrings + numStrings) % numStrings;
    endStringIdx = ((upperLeftStringIdx + widthInStrings - 1) % numStrings + numStrings) % numStrings;
    startPixelIdx = (upperLeftPixelIdx % pixelsPerString + pixelsPerString) % pixelsPerString;
    endPixelIdx = ((upperLeftPixelIdx + heightInPixels - 1) % pixelsPerString + pixelsPerString) % pixelsPerString;

    return true;
}


void IndicatorRegion::fillRegion(const HsvPixel& color)
{
    // We're doing the funky do/while stuff to support wraparound from the last
    // string to the first string (i.e., startStringIdx > endStringIdx).

    //logger.logMsg(LOG_DEBUG, "startStringIdx=" + to_string(startStringIdx) + ", endStringIdx=" + to_string(endStringIdx));
    unsigned int i = startStringIdx;
    do {
        if (i >= numStrings) {
            i = 0;
        }
        unsigned int j = startPixelIdx;
        do {
            if (j >= pixelsPerString) {
                j = 0;
            }
            //string hsvStr;
            //hsvPixelToString(foregroundColor, hsvStr);
            //logger.logMsg(LOG_DEBUG, "setting (" + to_string(i) + ", " + to_string(j) + ") to " + hsvStr);
            (*coneStrings)[i][j] = color;
        } while (j++ != endPixelIdx);
    } while (i++ != endStringIdx);
}

