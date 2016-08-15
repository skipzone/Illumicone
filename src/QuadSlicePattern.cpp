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

#include <iostream>
#include "Widget.h"
#include "WidgetChannel.h"
#include "Pattern.h"
#include "QuadSlicePattern.h"
#include "WidgetFactory.h"
#include "illumiconeTypes.h"

using namespace std;

bool QuadSlicePattern::initPattern(int numStrings, int pixelsPerString, int priority)
{
    this->numStrings = numStrings;
    this->pixelsPerString = pixelsPerString;
    cout << "QuadSlicePattern priority: " << priority << endl;
    this->priority = priority;
    this->pixelArray.resize(numStrings, std::vector<opc_pixel_t>(pixelsPerString));
    this->name = "QuadSlicePattern";

    this->isActive = 0;
    this->opacity = 80;
    return true;
}

bool QuadSlicePattern::initWidgets(int numWidgets, int channelsPerWidget)
{
    int i;

    for (i = 0; i < numWidgets; i++) {
        Widget* newWidget = widgetFactory(WidgetId::steps);
        widgets.emplace_back(newWidget);
        newWidget->init(false);
    }

    return true;
}

bool QuadSlicePattern::update()
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
    int hadActivity = 0;

    // TODO:  change class to have just one widget
    auto widget = widgets[0];

    widget->moveData();

    for (auto&& channel : widget->getChannels()) {

        int firstStringIdx;
        int lastStringIdx;
        int firstPixelIdx = 0;
        int lastPixelIdx = 0;
        switch (channel->getChannelNumber()) {
            case 0:
            case 1:
                firstStringIdx = (NUM_STRINGS / 4) * channel->getChannelNumber();
                lastStringIdx = firstStringIdx + (NUM_STRINGS / 4) - 1;
                firstPixelIdx = pixelsPerString / 3 + 1;
                lastPixelIdx = pixelsPerString - 1;
                break;

            case 2:     // top platform
                firstStringIdx = 0;
                lastStringIdx = NUM_STRINGS - 1;
                firstPixelIdx = 0;
                lastPixelIdx = pixelsPerString / 3;
                break;

            case 3:
            case 4:
                firstStringIdx = (NUM_STRINGS / 4) * (channel->getChannelNumber() - 1);
                lastStringIdx = firstStringIdx + (NUM_STRINGS / 4) - 1;
                firstPixelIdx = pixelsPerString / 3 + 1;
                lastPixelIdx = pixelsPerString - 1;
                break;
        }
        //cout << "pixelArray.size() = " << pixelArray.size() << ", pixelArray[0].size() = " << pixelArray[0].size() << endl;
        //cout << "pixelsPerString = " << pixelsPerString << endl;
        //cout << "firstStringIdx: " << firstStringIdx << endl;
        //cout << "lastStringIdx: " << lastStringIdx << endl;
        //cout << "firstPixelIdx: " << firstPixelIdx << endl;
        //cout << "lastPixelIdx: " << lastPixelIdx << endl;

        if (channel->getIsActive()) {
            hadActivity = 1;

            if (channel->getHasNewMeasurement()) {

                int curPos = channel->getPosition();

                cout << "Channel: " << channel->getChannelNumber() << endl;
                cout << "curPos: " << curPos << endl;

                if (curPos > 0) {
                    // TODO:  set the color depending on the measurement (where the footfall is on the step)
                    r = (uint8_t) (rand() % 128);
                    g = (uint8_t) (rand() % 128);
                    b = (uint8_t) (rand() % 128);
                }
                else {
                    r = 1;
                    g = 1;
                    b = 1;
                }
            
                for (int i = firstStringIdx; i <= lastStringIdx; i++) {
                    for (int j = firstPixelIdx; j <= lastPixelIdx; ++j) {
                        pixelArray[i][j].r = r;
                        pixelArray[i][j].g = g;
                        pixelArray[i][j].b = b;
                    }
                }
            }
        }
        else if (isActive) {
            // The channel is inactive now, so turn off that part of the pattern.
            cout << "Turning off for channel " << channel->getChannelNumber() << endl;
            for (int i = firstStringIdx; i <= lastStringIdx; i++) {
                for (int j = firstPixelIdx; j <= lastPixelIdx; ++j) {
                    pixelArray[i][j].r = 1;
                    pixelArray[i][j].g = 1;
                    pixelArray[i][j].b = 1;
                }
            }
        }
    }

    isActive = hadActivity;

    return true;
}
