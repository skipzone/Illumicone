#include <chrono>
#include <iostream>
#include <string>
//#include <thread>
#include <time.h>
///#include <vector>

#include "EyeWidget.h"
#include "illumiconeTypes.h"

using namespace std;


EyeWidget::EyeWidget()
{
    for (unsigned int i = 0; i < 8; ++i) {
        updateIntervalMs[i] = 0;
        lastUpdateMs[i] = 0;
    }

    updateIntervalMs[0] = 5000;
}


void EyeWidget::init()
{
    channels.push_back(make_shared<WidgetChannel>(0, this));
}


unsigned int EyeWidget::getId()
{
    return EyeWidget::id;
}


std::string EyeWidget::getName()
{
    //return EyeWidget::name;
    return "EyeWidget";
}


bool EyeWidget::moveData()
{
    using namespace std::chrono;

    milliseconds epochMs = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
    unsigned int nowMs = epochMs.count();

    //cout << "---------- nowMs = " << nowMs << endl;

    for (unsigned int i = 0; i < getChannelCount(); ++i) {
        //cout << "checking channel " << i << endl;
        if (updateIntervalMs[i] > 0 && nowMs - lastUpdateMs[i] > updateIntervalMs[i]) {
            //cout << "updating channel " << i << endl;
            int prevPos = channels[i]->getPreviousPosition();
            lastUpdateMs[i] = nowMs;
            cout << "Eye prevPos: " << prevPos << endl;
            if (prevPos == 0) {
                channels[i]->setPositionAndVelocity(1, 0);
            } else {
                channels[i]->setPositionAndVelocity(0, 0);
            }
            channels[i]->setIsActive(true);
            //cout << "updated channel " << i << endl;
        }
    }

    return true;
}

