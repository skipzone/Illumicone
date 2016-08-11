#include <iostream>
#include "Widget.h"
#include "WidgetChannel.h"
#include "Pattern.h"
#include "WidgetFactory.h"
#include "RainbowExplosionPattern.h"

using namespace std;

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
    int hadActivity = 0;
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
//                cout << "Updating widget's channel!" << endl;
                if (channel->getHasNewMeasurement() || channel->getIsActive()) {
                    // TODO: Do stuff
                }
            }
        }
    }

    isActive = hadActivity;
    return true;
}
