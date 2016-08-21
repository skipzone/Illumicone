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
#include "RainbowExplosionPattern.h"

using namespace std;

enum patternState {
    STATE_FIZZLE = 0,
    STATE_R,
    STATE_O,
    STATE_Y,
    STATE_G,
    STATE_B,
    STATE_I,
    STATE_V,
};

static patternState state;
static int colorPosition;
static int accumulator;

bool RainbowExplosionPattern::initPattern(int numStrings, int pixelsPerString, int priority)
{
    this->numStrings = numStrings;
    this->pixelsPerString = pixelsPerString;
    cout << "RainbowExplosionPattern priority: " << priority << endl;
    this->priority = priority;
    this->pixelArray.resize(numStrings, std::vector<opc_pixel_t>(pixelsPerString));
    this->name = "RainbowExplosionPattern";

    this->isActive = 0;
    this->opacity = 100;

    state = STATE_FIZZLE;

    for (auto&& pixels:pixelArray) {
        for (auto&& pixel:pixels) {
            pixel.r = 0;
            pixel.g = 0;
            pixel.b = 0;
        }
    }

    colorPosition = PIXELS_PER_STRING;
    accumulator = 0;

    return true;
}

bool RainbowExplosionPattern::initWidgets(int numWidgets, int channelsPerWidget)
{
    int i;
//    cout << "Init RGB Vertical Pattern Widgets!" << endl;

    for (i = 0; i < numWidgets; i++) {
        Widget* newWidget = widgetFactory(WidgetId::plunger);
        widgets.emplace_back(newWidget);
        newWidget->init(false);
    }

    return true;
}


bool RainbowExplosionPattern::update()
{
    bool hadActivity = false;
//    cout << "Updating Solid Black Pattern!" << endl;

    cout << "Updating rainbowExplosion" << endl;
    for (auto&& widget:widgets) {
//        cout << "Updating Solid Black Pattern widget!" << endl;
        // update active, position, velocity for each channel in widget
        widget->moveData();
        if (widget->getIsActive()) {
            cout << "Plunger is active!" << endl;
            for (auto&& channel:widget->getChannels()) {
                cout << "Got a channel" << endl;
//                cout << "Updating widget's channel!" << endl;
                if (channel->getHasNewMeasurement() || channel->getIsActive()) {
                    hadActivity = true;
                    // TODO: Do stuff
                    int curPos = channel->getPosition();
                    cout << "Plunger position: " << curPos << endl;

                    switch (state) {
                        case STATE_FIZZLE:
                            for (auto&& pixels:pixelArray) {
                                for (auto&& pixel:pixels) {
                                    pixel.r = 0;
                                    pixel.g = 0;
                                    pixel.b = 0;
                                }
                            }

                            //
                            // pump only "counts" if it registers above 900 p-p
                            // accumulator is set rand() mod 5 to get more of a "random"
                            // explosion response
                            //                               
                            if (curPos >= 800) {
                                if (accumulator > 20) {
                                    accumulator = rand() % 8;
                                    state = STATE_R;
                                } else {
                                    accumulator++;
                                }

                                for (auto&& pixels:pixelArray) {
                                    int randPixel = rand() % 8;
                                    for (int i = PIXELS_PER_STRING - randPixel; i < PIXELS_PER_STRING; i++) {
                                        pixels[i].r = 127;
                                        pixels[i].g = 0;
                                        pixels[i].b = 0;
                                    }
                                }
                            }
                            break;

                        case STATE_R:
                            for (int i = colorPosition; i < PIXELS_PER_STRING; i++) {
                                for (auto&& pixels:pixelArray) {
                                    pixels[i].r = 255;
                                    pixels[i].g = 0;
                                    pixels[i].b = 0;
                                }
                            }
                            if (colorPosition == 0) {
                                colorPosition = PIXELS_PER_STRING;
                                state = STATE_O;
                            }
                            colorPosition -= 4;

                            break;

                        case STATE_O:
                            for (int i = colorPosition; i < PIXELS_PER_STRING; i++) {
                                for (auto&& pixels:pixelArray) {
                                    pixels[i].r = 255;
                                    pixels[i].g = 127;
                                    pixels[i].b = 0;
                                }
                            }
                            if (colorPosition == 0) {
                                colorPosition = PIXELS_PER_STRING;
                                state = STATE_Y;
                            }
                            colorPosition -= 4;

                            break;

                        case STATE_Y:
                            for (int i = colorPosition; i < PIXELS_PER_STRING; i++) {
                                for (auto&& pixels:pixelArray) {
                                    pixels[i].r = 255;
                                    pixels[i].g = 255;
                                    pixels[i].b = 0;
                                }
                            }
                            if (colorPosition == 0) {
                                colorPosition = PIXELS_PER_STRING;
                                state = STATE_G;
                            }
                            colorPosition -= 4;

                            break;

                        case STATE_G:
                            for (int i = colorPosition; i < PIXELS_PER_STRING; i++) {
                                for (auto&& pixels:pixelArray) {
                                    pixels[i].r = 0;
                                    pixels[i].g = 255;
                                    pixels[i].b = 0;
                                }
                            }
                            if (colorPosition == 0) {
                                colorPosition = PIXELS_PER_STRING;
                                state = STATE_B;
                            }
                            colorPosition -= 4;

                            break;

                        case STATE_B:
                            for (int i = colorPosition; i < PIXELS_PER_STRING; i++) {
                                for (auto&& pixels:pixelArray) {
                                    pixels[i].r = 0;
                                    pixels[i].g = 0;
                                    pixels[i].b = 255;
                                }
                            }
                            if (colorPosition == 0) {
                                colorPosition = PIXELS_PER_STRING;
                                state = STATE_I;
                            }
                            colorPosition -= 4;

                            break;

                        case STATE_I:
                            for (int i = colorPosition; i < PIXELS_PER_STRING; i++) {
                                for (auto&& pixels:pixelArray) {
                                    pixels[i].r = 75;
                                    pixels[i].g = 0;
                                    pixels[i].b = 130;
                                }
                            }
                            if (colorPosition == 0) {
                                colorPosition = PIXELS_PER_STRING;
                                state = STATE_V;
                            }
                            colorPosition -= 4;

                            break;

                        case STATE_V:
                            for (int i = colorPosition; i < PIXELS_PER_STRING; i++) {
                                for (auto&& pixels:pixelArray) {
                                    pixels[i].r = 148;
                                    pixels[i].g = 0;
                                    pixels[i].b = 211;
                                }
                            }
                            if (colorPosition == 0) {
                                colorPosition = PIXELS_PER_STRING;
                                state = STATE_FIZZLE;
                            }
                            colorPosition -= 4;

                            break;

                        default:
                            cout << "SOMETHING'S FUCKY: state in RainbowExplosionPattern" << endl;
                    }
                }
            }
        }
    }

    isActive = hadActivity;
    return true;
}
