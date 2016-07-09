#include <iostream>
#include "Widget.h"
#include "WidgetChannel.h"
#include "Pattern.h"
#include "QuadSlicePattern.h"
#include "WidgetFactory.h"

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
    int i, ii;

    for (i = 0; i < numWidgets; i++) {
        widgets.emplace_back(widgetFactory(3));
        widgets[i]->init(channelsPerWidget);
        ii = 0;
        for (auto&& channel:widgets[i]->channels) {
            channel.initChannel(ii, 0, 0);
            ii++;
        }
    }

    return true;
}

bool QuadSlicePattern::update()
{
    int i;
    int hadActivity = 0;
    uint8_t r;
    uint8_t g;
    uint8_t b;

    for (auto&& widget:widgets) {
        // update active, position, velocity for each channel in widget
        widget->moveData();
        for (auto&& channel:widget->channels) {
            if (channel.isActive) {
                hadActivity = 1;
                switch (channel.number) {
                    case 0:
                        //
                        // First Stair, Right Side
                        //
                        if (channel.position != channel.prevPosition) {
                            if (channel.position) {
                                // get the first quarter of the strings to iterate through
                                for (i = 0; i < NUM_STRINGS / 4; i++) {
//                                    r = rand() % (255 - 0) + 0;
//                                    g = rand() % (255 - 0) + 0;
//                                    b = rand() % (255 - 0) + 0;
                                    r = 64;
                                    g = 0;
                                    b = 64;
                                    for (auto&& pixel:pixelArray[i]) {
                                        pixel.r = r;
                                        pixel.g = g;
                                        pixel.b = b;
                                    }
                                }
                            } else {
                                for (i = 0; i < NUM_STRINGS / 4; i++) {
                                    for (auto&& pixel:pixelArray[i]) {
                                        pixel.r = 0;
                                        pixel.g = 0;
                                        pixel.b = 0;
                                    }
                                }
                            }
                        }

                        break;

                    case 1:
                        //
                        // Second Stair, Right Side
                        //
                        if (channel.position != channel.prevPosition) {
                            if (channel.position) {
                                // get the second third of the strings to iterate through
                                for (i = (NUM_STRINGS / 4); i < (NUM_STRINGS / 4) * 2; i++) {
//                                    r = rand() % (255 - 0) + 0;
//                                    g = rand() % (255 - 0) + 0;
//                                    b = rand() % (255 - 0) + 0;
                                    r = 64;
                                    g = 64;
                                    b = 0;
                                    for (auto&& pixel:pixelArray[i]) {
                                        pixel.r = r;
                                        pixel.g = g;
                                        pixel.b = b;
                                    }
                                }
                            } else {
                                for (i = (NUM_STRINGS / 4); i < (NUM_STRINGS / 4) * 2; i++) {
                                    for (auto&& pixel:pixelArray[i]) {
                                        pixel.r = 0;
                                        pixel.g = 0;
                                        pixel.b = 0;
                                    }
                                }
                            }
                        }

                        break;

                    case 2:
                        //
                        // First Stair, Left Side
                        //
                        if (channel.position != channel.prevPosition) {
                            if (channel.position) {
                                // get the third third of the strings to iterate through
                                for (i = ((NUM_STRINGS / 4) * 2); i < ((NUM_STRINGS / 4) * 3); i++) {
//                                    r = rand() % (255 - 0) + 0;
//                                    g = rand() % (255 - 0) + 0;
//                                    b = rand() % (255 - 0) + 0;
                                    r = 0;
                                    g = 64;
                                    b = 64;
                                    for (auto&& pixel:pixelArray[i]) {
                                        pixel.r = r;
                                        pixel.g = g;
                                        pixel.b = b;
                                    }
                                }
                            } else {
                                for (i = ((NUM_STRINGS / 4) * 2); i < ((NUM_STRINGS / 4) * 3); i++) {
                                    for (auto&& pixel:pixelArray[i]) {
                                        pixel.r = 0;
                                        pixel.g = 0;
                                        pixel.b = 0;
                                    }
                                }
                            }
                        }

                        break;

                    case 3:
                        //
                        // Second Stair, Left Side
                        //
                        if (channel.position != channel.prevPosition) {
                            if (channel.position) {
                                // get the third third of the strings to iterate through
                                for (i = ((NUM_STRINGS / 4) * 3); i < NUM_STRINGS; i++) {
//                                    r = rand() % (255 - 0) + 0;
//                                    g = rand() % (255 - 0) + 0;
//                                    b = rand() % (255 - 0) + 0;
                                    r = 64;
                                    g = 64;
                                    b = 64;
                                    for (auto&& pixel:pixelArray[i]) {
                                        pixel.r = r;
                                        pixel.g = g;
                                        pixel.b = b;
                                    }
                                }
                            } else {
                                for (i = ((NUM_STRINGS / 4) * 3); i < NUM_STRINGS; i++) {
                                    for (auto&& pixel:pixelArray[i]) {
                                        pixel.r = 0;
                                        pixel.g = 0;
                                        pixel.b = 0;
                                    }
                                }
                            }
                        }

                        break;

                    default:
                        // shouldn't get here, solid black uses the eye widget which
                        // should only have one channel.
                        cout << "SOMETHING'S FUCKY : channel number for Solid Black Pattern widget" << endl;
                        break;
                }
            }
        }
    }

    //
    // set pattern activity flag
    //
    isActive = hadActivity;
    return true;
}
