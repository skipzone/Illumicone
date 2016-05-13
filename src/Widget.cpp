#include <iostream>
#include <vector>

#include "Widget.h"
#include "WidgetChannel.h"

using namespace std;

Widget::Widget(int numChannels) :
    channels(numChannels)
{
}
