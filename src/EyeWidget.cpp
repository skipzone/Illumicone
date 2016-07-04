#include <iostream>
#include <vector>

#include "EyeWidget.h"
#include "illumiconeTypes.h"

using namespace std;

bool EyeWidget::moveData()
{
    for (auto&& channel:channels) {
//        cout << "moveData EyeWidget Channel " << channel.number << endl;
        switch (channel.number) {
            case 0:
                channel.velocity = 0;
                channel.prevPosition = channel.position;
                if (channel.isActive) {
                    channel.isActive = 0;
                } else {
//                    channel.isActive = 1;
                }

                if (channel.position) {
                    channel.position = 0;
                } else {
                    channel.position = 1;
                }

            default:
                return false;
        }
    }

    return true;
}
