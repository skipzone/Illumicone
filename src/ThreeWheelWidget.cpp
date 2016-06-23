#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <regex>

#include "ThreeWheelWidget.h"

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
        cout << "moveData Channel!" << endl;
        channel.isActive = true;
        channel.position = 1;
        channel.velocity = 0;
    }
//    std::ifstream uart_stream("/dev/ttyUSB0"); 
//    std::string line;
//    regex re(",");
//    int i;
//    int dummy;
//
//    cout << "Moving data - -" << endl;
//    getline(uart_stream, line);
//    cout << line << endl;
//
//    sregex_token_iterator iter(line.begin(), line.end(), re, -1);
//
//    for (i = 0; i < channels.size(); i++) {
//        dummy = stoi(iter->str());
//        channels[i].isActive = stoi(iter->str());
//        dummy = stoi(iter->str());
//        channels[i].position = stoi(iter->str());
//    }
//
//    return true;
}
