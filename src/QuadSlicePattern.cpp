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
        newWidget->init();
    }

    return true;
}

bool QuadSlicePattern::update()
{
//    int i;
//    int hadActivity = 0;
//    uint8_t r;
//    uint8_t g;
//    uint8_t b;
//
//    for (auto&& widget:widgets) {
//        // update active, position, velocity for each channel in widget
//        widget->moveData();
//        for (auto&& channel:widget->channels) {
//            if (channel.isActive) {
//                hadActivity = 1;
//                switch (channel.number) {
//                    case 0:
//                        //
//                        // First Stair, Right Side
//                        //
//                        if (channel.position != channel.prevPosition) {
//                            if (channel.position) {
//                                // get the first quarter of the strings to iterate through
//                                for (i = 0; i < NUM_STRINGS / 4; i++) {
////                                    r = rand() % (255 - 0) + 0;
////                                    g = rand() % (255 - 0) + 0;
////                                    b = rand() % (255 - 0) + 0;
//                                    r = 64;
//                                    g = 0;
//                                    b = 64;
//                                    for (auto&& pixel:pixelArray[i]) {
//                                        pixel.r = r;
//                                        pixel.g = g;
//                                        pixel.b = b;
//                                    }
//                                }
//                            } else {
//                                for (i = 0; i < NUM_STRINGS / 4; i++) {
//                                    for (auto&& pixel:pixelArray[i]) {
//                                        pixel.r = 0;
//                                        pixel.g = 0;
//                                        pixel.b = 0;
//                                    }
//                                }
//                            }
//                        }
//
//                        break;
//
//                    case 1:
//                        //
//                        // Second Stair, Right Side
//                        //
//                        if (channel.position != channel.prevPosition) {
//                            if (channel.position) {
//                                // get the second third of the strings to iterate through
//                                for (i = (NUM_STRINGS / 4); i < (NUM_STRINGS / 4) * 2; i++) {
////                                    r = rand() % (255 - 0) + 0;
////                                    g = rand() % (255 - 0) + 0;
////                                    b = rand() % (255 - 0) + 0;
//                                    r = 64;
//                                    g = 64;
//                                    b = 0;
//                                    for (auto&& pixel:pixelArray[i]) {
//                                        pixel.r = r;
//                                        pixel.g = g;
//                                        pixel.b = b;
//                                    }
//                                }
//                            } else {
//                                for (i = (NUM_STRINGS / 4); i < (NUM_STRINGS / 4) * 2; i++) {
//                                    for (auto&& pixel:pixelArray[i]) {
//                                        pixel.r = 0;
//                                        pixel.g = 0;
//                                        pixel.b = 0;
//                                    }
//                                }
//                            }
//                        }
//
//                        break;
//
//                    case 2:
//                        //
//                        // First Stair, Left Side
//                        //
//                        if (channel.position != channel.prevPosition) {
//                            if (channel.position) {
//                                // get the third third of the strings to iterate through
//                                for (i = ((NUM_STRINGS / 4) * 2); i < ((NUM_STRINGS / 4) * 3); i++) {
////                                    r = rand() % (255 - 0) + 0;
////                                    g = rand() % (255 - 0) + 0;
////                                    b = rand() % (255 - 0) + 0;
//                                    r = 0;
//                                    g = 64;
//                                    b = 64;
//                                    for (auto&& pixel:pixelArray[i]) {
//                                        pixel.r = r;
//                                        pixel.g = g;
//                                        pixel.b = b;
//                                    }
//                                }
//                            } else {
//                                for (i = ((NUM_STRINGS / 4) * 2); i < ((NUM_STRINGS / 4) * 3); i++) {
//                                    for (auto&& pixel:pixelArray[i]) {
//                                        pixel.r = 0;
//                                        pixel.g = 0;
//                                        pixel.b = 0;
//                                    }
//                                }
//                            }
//                        }
//
//                        break;
//
//                    case 3:
//                        //
//                        // Second Stair, Left Side
//                        //
//                        if (channel.position != channel.prevPosition) {
//                            if (channel.position) {
//                                // get the third third of the strings to iterate through
//                                for (i = ((NUM_STRINGS / 4) * 3); i < NUM_STRINGS; i++) {
////                                    r = rand() % (255 - 0) + 0;
////                                    g = rand() % (255 - 0) + 0;
////                                    b = rand() % (255 - 0) + 0;
//                                    r = 64;
//                                    g = 64;
//                                    b = 64;
//                                    for (auto&& pixel:pixelArray[i]) {
//                                        pixel.r = r;
//                                        pixel.g = g;
//                                        pixel.b = b;
//                                    }
//                                }
//                            } else {
//                                for (i = ((NUM_STRINGS / 4) * 3); i < NUM_STRINGS; i++) {
//                                    for (auto&& pixel:pixelArray[i]) {
//                                        pixel.r = 0;
//                                        pixel.g = 0;
//                                        pixel.b = 0;
//                                    }
//                                }
//                            }
//                        }
//
//                        break;
//
//                    default:
//                        // shouldn't get here, solid black uses the eye widget which
//                        // should only have one channel.
//                        cout << "SOMETHING'S FUCKY : channel number for Solid Black Pattern widget" << endl;
//                        break;
//                }
//            }
//        }
//    }
//
//    //
//    // set pattern activity flag
//    //
//    isActive = hadActivity;

    uint8_t r;
    uint8_t g;
    uint8_t b;
    int hadActivity = 0;

    for (auto&& widget:widgets) {
        widget->moveData();

        for (auto&& channel:widget->getChannels()) {
            if (channel->getIsActive()) {
                hadActivity = 1;
            }
            if (channel->getHasNewMeasurement()) {
//                int curPos = channel->getPosition();
                int initIndex;
                int lastIndex;
                if (channel->getChannelNumber() < 4) {
                    initIndex = (NUM_STRINGS / 4) * channel->getChannelNumber();
                    lastIndex = (NUM_STRINGS / 4) * (channel->getChannelNumber() + 1);
                } else {
                    initIndex = 0;
                    lastIndex = NUM_STRINGS;
                }
//                cout << "Channel: " << channel->getChannelNumber() << endl;
//                cout << "curPos: " << curPos << endl;
//                cout << "initIndex: " << initIndex << endl;
//                cout << "lastIndex: " << lastIndex << endl;

//                r = (uint8_t)(rand() % 255);
//                g = (uint8_t)(rand() % 255);
//                b = (uint8_t)(rand() % 255);
                r = 64;
                g = 0;
                b = 64;

                if (channel->getChannelNumber() < 4) {
                    for (int i = initIndex; i < lastIndex; i++) {
                        for (auto&& pixel:pixelArray[i]) {
                            pixel.r = r;
                            pixel.g = g;
                            pixel.b = b;
                        }
                    }
                }

//                else {
//                    for (auto&& pixels:pixelArray) {
//                        for (auto&& pixel:pixels) {
//                            pixel.r = 0;
//                            pixel.g = 0;
//                            pixel.b = 0;
//                        }
//                    }
//                }

            }
        }
    }

    isActive = hadActivity;

    return true;
}
