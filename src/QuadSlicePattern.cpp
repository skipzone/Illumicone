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

    for (auto&& widget:widgets) {
        widget->moveData();

        for (auto&& channel:widget->getChannels()) {
            if (channel->getIsActive()) {
                hadActivity = 1;
            }

            if (channel->getHasNewMeasurement()) {
                int initIndex;
                int lastIndex;
                int curPos = channel->getPosition();
//                cout << "Quad slice channel " << channel->getChannelNumber() << " has new measurement!" << endl;
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

                r = (uint8_t)(rand() % 255);
                g = (uint8_t)(rand() % 255);
                b = (uint8_t)(rand() % 255);
//                r = 255;
//                g = 0;
//                b = 255;

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
