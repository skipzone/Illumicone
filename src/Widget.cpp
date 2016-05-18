#include <iostream>
#include <vector>

#include "Widget.h"
#include "WidgetChannel.h"

using namespace std;

Widget::Widget(int numChannels)
{
    int i;
    for (i = 0; i < numChannels; i++) {
        channels.emplace_back(0, 0);
    }
}
