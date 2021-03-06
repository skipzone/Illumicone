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
#include "WidgetFactory.h"
#include "TwistPattern.h"

using namespace std;

bool TwistPattern::initPattern(unsigned int numStrings, unsigned int pixelsPerString, int priority)
{
//    cout << "Init Solid Black Pattern!" << endl;
    this->numStrings = numStrings;
    this->pixelsPerString = pixelsPerString;
    this->priority = priority;
    this->pixelArray.resize(numStrings, std::vector<opc_pixel_t>(pixelsPerString));
    this->name = "TwistPattern";

    this->isActive = 0;
    this->opacity = 100;
    return true;
}

bool TwistPattern::initWidgets(int numWidgets, int channelsPerWidget)
{
    int i, ii;
//    cout << "Init RGB Vertical Pattern Widgets!" << endl;

    for (i = 0; i < numWidgets; i++) {
        Widget* newWidget = widgetFactory(WidgetId::shirleysWeb);
        widgets.emplace_back(newWidget);
        newWidget->init();
    }

    return true;
}

/*
 * This pattern is different than the others.  Instead of writing RGB values
 * to the pixelArray, the number of strings to shift the pixels is written.
 * 
 * The number of strings to shift the pixels by is determined by the velocity of
 * the associated widget.
 *
 * The velocity should range 1-36, 36 being the number of strings meaning the 
 * most the pixels can be shifted is completely around the cone.  When velocity
 * is 1, the first 36th of the lights on a string will be shifted by one.  When
 * it is 2, the first 36th of the lights on a string (from the top of the cone)
 * will be shifted by 2, and the second 36th is shifted by 1.  If it is 3, the
 * first 36th are shifted by 3, the second by 2, and the third by 1.  And so forth.
 *
 * So, the calculation for shift amounts will be done like:
 *
 * // for each column
 * for (auto&& pixels:pixelArray) {
 *     for (i = 0; i < velocity; i++) {
 *         for (ii = velocity * pixelsPerString*3/numStrings; ii < pixelsPerString*3/numStrings + pixelsPerString*3/numStrings; ii+=3) {
 *              pixels[ii+0].r = velocity + 1;
 *              pixels[ii+0].g = velocity + 1;
 *              pixels[ii+0].b = velocity + 1;
 *
 *              pixels[ii+1].r = velocity + 1;
 *              pixels[ii+1].g = velocity + 1;
 *              pixels[ii+1].b = velocity + 1;
 *
 *              pixels[ii+2].r = velocity + 1;
 *              pixels[ii+2].g = velocity + 1;
 *              pixels[ii+2].b = velocity + 1;
 *         }
 *     } 
 * }
 */
bool TwistPattern::update()
{
    int hadActivity = 0;
    int shiftValues[pixelsPerString];
    int shiftGroup = 0;
    int pixelToShift = 0;
    int shiftAmount;
//    cout << "Updating Solid Black Pattern!" << endl;

    for (auto&& pixels:pixelArray) {
        for (auto&& pixel:pixels) {
            pixel.r = 0;
            pixel.g = 0;
            pixel.b = 0;
        }
    }

    for (auto&& widget:widgets) {
//        cout << "Updating Solid Black Pattern widget!" << endl;
        // update active, position, velocity for each channel in widget
        widget->moveData();
        if (widget->getIsActive()) {
            for (auto&& channel:widget->getChannels()) {
    //            cout << "Updating widget's channel!" << endl;
                if (channel->getHasNewMeasurement() || channel->getIsActive()) {
                    hadActivity = 1;
                    int curVel = channel->getVelocity();
                    shiftAmount = curVel;

                    for (shiftGroup = 0; shiftGroup < curVel; shiftGroup++) {
                        for (pixelToShift = (pixelsPerString / numStrings) * shiftGroup;
                                pixelToShift < (pixelsPerString / numStrings) * (shiftGroup + 1);
                                pixelToShift++) {
                            shiftValues[pixelToShift] = shiftAmount;
                            
                        }
                        shiftAmount--;
                    }

                    for (auto&& pixels:pixelArray) {
                        int i = 0;
                        for (auto&& pixel:pixels) {
                            pixel.r = shiftValues[i];
                            pixel.g = shiftValues[i];
                            pixel.b = shiftValues[i];
                            i++;
                        }
                    }
                }
            }
        }
    }

    isActive = hadActivity;
    return true;
}
