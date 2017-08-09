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

#include <string>

#include "illumiconePixelUtility.h"
#include "log.h"
#include "SimpleBlockIndicator.h"


using namespace std;


void SimpleBlockIndicator::turnOffImmediately()
{
    int i = startStringIdx;
    do {
        if (i >= numStrings) {
            i = 0;
        }
        int j = startPixelIdx;
        do {
            if (j >= pixelsPerString) {
                j = 0;
            }
            (*coneStrings)[i][j] = backgroundColor;
        } while (j++ != endPixelIdx);
    } while (i++ != endStringIdx);
}


void SimpleBlockIndicator::turnOnImmediately()
{
    //logMsg(LOG_DEBUG, "startStringIdx=" + to_string(startStringIdx) + ", endStringIdx=" + to_string(endStringIdx));
    int i = startStringIdx;
    do {
        if (i >= numStrings) {
            i = 0;
        }
        int j = startPixelIdx;
        do {
            if (j >= pixelsPerString) {
                j = 0;
            }
            //string hsvStr;
            //hsvPixelToString(foregroundColor, hsvStr);
            //logMsg(LOG_DEBUG, "setting (" + to_string(i) + ", " + to_string(j) + ") to " + hsvStr);
            (*coneStrings)[i][j] = foregroundColor;
        } while (j++ != endPixelIdx);
    } while (i++ != endStringIdx);
}

