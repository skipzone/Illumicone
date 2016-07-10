#include <iostream>
#include <vector>
#include <chrono>

#include <string>
#include <time.h>

#include "StairWidget.h"
#include "illumiconeTypes.h"

using namespace std;


StairWidget::StairWidget()
{
    for (unsigned int i = 0; i < 8; ++i) {
        updateIntervalMs[i] = 0;
        lastUpdateMs[i] = 0;
    }

    updateIntervalMs[0] = 3000;
    updateIntervalMs[1] = 3000;
    updateIntervalMs[2] = 3000;
    updateIntervalMs[3] = 3000;
    updateIntervalMs[4] = 3000;
    updateIntervalMs[5] = 0;
    updateIntervalMs[6] = 0;
    updateIntervalMs[7] = 0;
}


void StairWidget::init()
{
    for (int i = 0; i < 5; ++i) {
        channels.push_back(make_shared<WidgetChannel>(i, this));
    }
}


unsigned int StairWidget::getId()
{
    return StairWidget::id;
}


std::string StairWidget::getName()
{
    //return ThreeWheelWidget::name;
    return "StairWidget";
}


bool StairWidget::moveData()
{
//    for (auto&& channel:channels) {
//        switch (channel.number) {
//            case 0:
//                channel.velocity = 0;
//                channel.prevPosition = channel.position;
//                channel.isActive = 1;
//                // position = 1 means stair is being stepped on
//                channel.position = 1;
//
//                break;
//
//            case 1:
//                channel.velocity = 0;
//                channel.prevPosition = channel.position;
//                channel.isActive = 1;
//                // position = 1 means stair is being stepped on
//                channel.position = 1;
//
//                break;
//
//            case 2:
//                channel.velocity = 0;
//                channel.prevPosition = channel.position;
//                channel.isActive = 1;
//                // position = 1 means stair is being stepped on
//                channel.position = 1;
//
//                break;
//
//            case 3:
//                channel.velocity = 0;
//                channel.prevPosition = channel.position;
//                channel.isActive = 1;
//                channel.position = 1;
//
//                break;
//
//            default:
//                return false;
//        }
//    }
    using namespace std::chrono;

    milliseconds epochMs = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
    unsigned int nowMs = epochMs.count();

    //cout << "---------- nowMs = " << nowMs << endl;

    for (unsigned int i = 0; i < getChannelCount(); ++i) {
//        cout << "checking channel " << i << endl;
        if (updateIntervalMs[i] > 0 && nowMs - lastUpdateMs[i] > updateIntervalMs[i]) {
//            cout << "updating channel " << i << endl;
            lastUpdateMs[i] = nowMs;
            channels[i]->setPositionAndVelocity((channels[i]->getPreviousPosition() + 1) % 3, 0);
            channels[i]->setIsActive(true);
//            cout << "updated channel " << i << endl;
        }
    }


    return true;
}
