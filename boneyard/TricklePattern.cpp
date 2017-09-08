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
#include <unistd.h>
#include "Widget.h"
#include "WidgetChannel.h"
#include "Pattern.h"
#include "WidgetFactory.h"
#include "WidgetId.h"
#include "TricklePattern.h"

using namespace std;


TricklePattern::~TricklePattern()
{
    delete [] tricklePositions;
}


bool TricklePattern::initPattern(unsigned int numStrings, unsigned int pixelsPerString, int priority)
{
    this->numStrings = numStrings;
    this->pixelsPerString = pixelsPerString;
    cout << "TricklePattern priority: " << priority << endl;
    this->priority = priority;
    this->pixelArray.resize(numStrings, std::vector<opc_pixel_t>(pixelsPerString));
    this->name = "TricklePattern";

    this->isActive = 0;
    this->opacity = 100;

    tricklePositions = new unsigned int[numStrings];
    for (auto&& pos : tricklePositions) {
        pos = 0;
    }

    return true;
}


bool TricklePattern::initWidgets(int numWidgets, int channelsPerWidget)
{
    int i;
//    cout << "Init RGB Vertical Pattern Widgets!" << endl;

    for (i = 0; i < numWidgets; i++) {
        Widget* newWidget = widgetFactory(WidgetId::rainstick);
        widgets.emplace_back(newWidget);
        newWidget->init(false);
    }

    return true;
}


bool TricklePattern::update()
{
    int hadActivity = 0;
    int stringNum = 0;
    int pixelsToLight = 0;
    int curPos = 190;
//    cout << "Updating Solid Black Pattern!" << endl;

    for (auto&& pixels : pixelArray) {
        for (auto&& pixel : pixels) {
            pixel.r = 0;
            pixel.g = 0;
            pixel.b = 0;
        }
    }

//    for (auto&& widget : widgets) {
////        cout << "Updating Solid Black Pattern widget!" << endl;
//        // update active, position, velocity for each channel in widget
//        widget->moveData();
//        if (widget->getIsActive()) {
//            for (auto&& channel:widget->getChannels()) {
////                cout << "Updating widget's channel!" << endl;
//                if (channel->getHasNewMeasurement() || channel->getIsActive()) {
//                    // TODO: Do stuff
//                    hadActivity = 1;
//                    float curPos = (float)(channel->getPosition());

                    //
                    // accelerometer was flipped 180 degrees...not sure if this
                    // value will be what comes from the widget
                    //
                    // Need some way to track the LED position.  Maybe we could
                    // just do shifts on the previous array?
                    //
                    if (curPos >= 180) {
                        for (auto&& pos : tricklePositions) {
                            if (pos < pixelsPerString) {
                                pos += rand() % 2;
                            }
                        }
                    }

                    for (auto&& col : pixelArray) {
                        cout << "Updating string " << stringNum << "with position  " << tricklePositions[stringNum] << endl;
                        for (int i = 0; i < tricklePositions[stringNum]; i++) {
                            col[i].r = 51;
                            col[i].g = 204;
                            col[i].b = 255;
                        }
                        stringNum++;
                    }

//                }
//            }
//        }
//    }

    isActive = hadActivity;
    return true;
}
