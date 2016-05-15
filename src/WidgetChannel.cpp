#include <stdbool.h>
#include <iostream>
#include <vector>

#include "WidgetChannel.h"

using namespace std;

WidgetChannel::WidgetChannel(int initialPosition, int initialVelocity) :
    position(initialPosition),
    velocity(initialVelocity),
    isActive(false)
{
}
