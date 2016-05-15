#include <iostream>
#include <vector>

#include "Widget.h"
#include "WidgetChannel.h"

using namespace std;

Widget::Widget(int numChannels, int initialPosition, int initialVelocity) :
    channels()
{
    int i;
    for (i = 0; i < numChannels; i++) {
        channels.emplace_back(initialPosition, initialVelocity);
    }
}
