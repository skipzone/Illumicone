#include <iostream>
#include "Widget.h"
#include "WidgetChannel.h"
#include "Pattern.h"
#include "WidgetFactory.h"
#include "SparklePattern.h"

using namespace std;

bool SparklePattern::initPattern(int numStrings, int pixelsPerString, int priority)
{
//    cout << "Init Solid Black Pattern!" << endl;
    this->numStrings = numStrings;
    this->pixelsPerString = pixelsPerString;
    cout << "SparklePattern priority: " << priority << endl;
    this->priority = priority;
    this->pixelArray.resize(numStrings, std::vector<opc_pixel_t>(pixelsPerString));
    this->name = "SparklePattern";

    this->isActive = 0;
    this->opacity = 100;
    return true;
}

bool SparklePattern::initWidgets(int numWidgets, int channelsPerWidget)
{
    int i, ii;
//    cout << "Init RGB Vertical Pattern Widgets!" << endl;

    for (i = 0; i < numWidgets; i++) {
        Widget* newWidget = widgetFactory(WidgetId::hypnotyzer);
        widgets.emplace_back(newWidget);
        newWidget->init(false);
    }

    return true;
}

bool SparklePattern::update()
{
    int i;
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
                    hadActivity = 1;
                    float curVel = (float)(channel->getVelocity());
                    float velocityPercentage = curVel / 600.0;
                    // at max velocity, only half of the pixels on each string sparkle
                    float numPixelsToSparkle = (velocityPercentage * (float)PIXELS_PER_STRING / 2);

//                    cout << "curVel: " << curVel << endl;
//                    cout << "velocityPercentage: " << velocityPercentage << endl;
//                    cout << "numPixelsToSparkle: " << numPixelsToSparkle << endl;

                    for (auto&& pixels:pixelArray) {
                        for (int i = 0; i < (int)numPixelsToSparkle; i++) {
                            int randPos = rand() % PIXELS_PER_STRING;
                            pixels[randPos].r = rand() % 255;
                            pixels[randPos].g = rand() % 255;
                            pixels[randPos].b = rand() % 255;
                        }
                    }
                }
            }
        }
    }

    isActive = hadActivity;
    return true;
}
