#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>

#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include "ThreeWheelWidget"


using namespace std;



/***********
 * Helpers *
 ***********/

const string getTimestamp()
{
    using namespace std::chrono;

    milliseconds epochMs = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
    int ms = epochMs.count() % 1000;
    time_t now = epochMs.count() / 1000;

    struct tm tmStruct = *localtime(&now);
    char buf[20];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tmStruct);

    stringstream sstr;
    sstr << buf << "." << setfill('0') << setw(3) << ms << ":  ";

    string str = sstr.str();
    return str;
}



/*********************************************
 * Initialization, Run Loop, and Entry Point *
 *********************************************/

void runLoop()
{

    while (1) {

        // There are no payloads to process, so give other threads a chance to run.
        this_thread::yield();
    }
}


int main(int argc, char** argv)
{
    runLoop();
    
    return 0;
}

