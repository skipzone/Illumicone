#include <iostream>
#include "Widget.h"
#include "WidgetChannel.h"
#include "Pattern.h"
#include "WidgetFactory.h"
#include "TwistPattern.h"

using namespace std;

bool TwistPattern::initPattern(int numStrings, int pixelsPerString, int priority)
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
        widgets.emplace_back(widgetFactory(4));
        widgets[i]->init(channelsPerWidget);
        ii = 0;
        for (auto&& channel:widgets[i]->channels) {
            channel.initChannel(ii, 0, 0);
            ii++;
        }
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
 *         for (ii = velocity * PIXELS_PER_STRING*3/NUM_STRINGS; ii < PIXELS_PER_STRING*3/NUM_STRINGS + PIXELS_PER_STRING*3/NUM_STRINGS; ii+=3) {
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
    int i, ii;
    int hadActivity = 0;
//    cout << "Updating Solid Black Pattern!" << endl;

    for (auto&& widget:widgets) {
//        cout << "Updating Solid Black Pattern widget!" << endl;
        // update active, position, velocity for each channel in widget
        widget->moveData();
        for (auto&& channel:widget->channels) {
//            cout << "Updating widget's channel!" << endl;
            if (channel.isActive) {
                hadActivity = 1;
                switch (channel.number) {
                    case 0:
                        for (auto&& pixels:pixelArray) {
                            for (i = 0; i < channel.velocity; i++) {
                                for (ii = i * PIXELS_PER_STRING*3/NUM_STRINGS;
                                        ii < PIXELS_PER_STRING*3/NUM_STRINGS + PIXELS_PER_STRING*3/NUM_STRINGS;
                                        ii+=3) {
                                    pixels[ii+0].r = i + 1;
                                    pixels[ii+0].g = i + 1;
                                    pixels[ii+0].b = i + 1;
                         
                                    pixels[ii+1].r = i + 1;
                                    pixels[ii+1].g = i + 1;
                                    pixels[ii+1].b = i + 1;

                                    pixels[ii+2].r = i + 1;
                                    pixels[ii+2].g = i + 1;
                                    pixels[ii+2].b = i + 1;
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

    isActive = hadActivity;
    return true;
}
