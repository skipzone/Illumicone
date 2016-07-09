#include <iostream>
#include <vector>

#include "RotaryWidget.h"
#include "illumiconeTypes.h"

using namespace std;

bool RotaryWidget::moveData()
{
    for (auto&& channel:channels) {
        switch (channel.number) {
            case 0:
                channel.velocity = 2;
                channel.prevPosition = channel.position;
                channel.isActive = 1;
                // position = 1 means stair is being stepped on
                channel.position = 1;

                break;

            default:
                return false;
        }
    }

    return true;
}
