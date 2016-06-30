#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <regex>

#include "ThreeWheelWidget.h"
#include "illumiconeTypes.h"

using namespace std;

//
// Usually, this function will call each channel's update() function, then
// read the position, velocity, and isActive.  But, since the 3-wheel
// provides a string from UART that contains information about all 3 wheels, we
// just parse that instead.
//
bool ThreeWheelWidget::moveData()
{
    for (auto&& channel:channels) {
//        cout << "moveData Channel " << channel.number << endl;
        switch (channel.number) {
            // ROSS: fill in with code to read widget data space
            case 0:
                channel.prevPosition = channel.position;
                channel.isActive = true;
                if (channel.position == NUM_STRINGS-1) {
                    channel.position = 0;
                } else {
                    channel.position++;
                }
                channel.velocity = 0;
                break;
            case 1:
                channel.prevPosition = channel.position;
                channel.isActive = false;
                if (channel.position == NUM_STRINGS-1) {
                    channel.position = 0;
                } else {
                    channel.position++;
                }
                channel.velocity = 0;
                break;
            case 2:
                channel.prevPosition = channel.position;
                channel.isActive = true;
                if (channel.position == NUM_STRINGS-1) {
                    channel.position = 0;
                } else {
                    channel.position++;
                }
                channel.velocity = 0;
                break;
            default:
                return false;
        }
    }
    return true;
}
