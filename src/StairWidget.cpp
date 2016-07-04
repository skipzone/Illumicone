#include <iostream>
#include <vector>

#include "StairWidget.h"
#include "illumiconeTypes.h"

using namespace std;

bool StairWidget::moveData()
{
    for (auto&& channel:channels) {
        switch (channel.number) {
            case 0:
                channel.velocity = 0;
                channel.prevPosition = channel.position;
                channel.isActive = 1;
                // position = 1 means stair is being stepped on
                channel.position = 1;

                break;

            case 1:
                channel.velocity = 0;
                channel.prevPosition = channel.position;
                channel.isActive = 1;
                // position = 1 means stair is being stepped on
                channel.position = 1;

                break;

            case 2:
                channel.velocity = 0;
                channel.prevPosition = channel.position;
                channel.isActive = 1;
                // position = 1 means stair is being stepped on
                channel.position = 1;

                break;

            case 3:
                channel.velocity = 0;
                channel.prevPosition = channel.position;
                channel.isActive = 1;
                channel.position = 1;

                break;

            default:
                return false;
        }
    }

    return true;
}
