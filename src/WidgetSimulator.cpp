#include <iostream>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cstdbool>

using namespace std;

int main()
{
    int threeWheelFd;
    int eyeFd;

    threeWheelFd = shm_open("/threeWheelArea", O_RDWR, 0);
    eyeFd = shm_open("/eyeArea", O_RDWR, 0);

    // index is:
    // Channel 0 [0]    isActive
    //           [1]    prevPosition
    //           [2]    position
    //           [3]    velocity
    // Channel 1 [4]    isActive
    //           [5]    prevPosition
    //           [6]    position
    //           [7]    velocity
    // Channel 2 [8]    isActive
    //           [9]    prevPosition
    //           [10]   position
    //           [11]   velocity
    ftruncate(threeWheelFd, sizeof(int)*4*3);

    // index is:
    // Channel 0 [0]    isActive
    //           [1]    prevPosition
    //           [2]    position
    //           [3]    velocity
    ftruncate(eyeFd, sizeof(int)*4*1);
    
    twPtr = mmap(NULL, sizeof(int)*4*3, PROT_READ | PROT_WRITE, MAP_SHARED, threeWheelFd, 0);
    if (twPtr == MAP_FAILED) {
        cout << "SOMETHING'S FUCKY: twPtr mmap failed" << endl;
    }

    eyPtr = mmap(NULL, sizeof(int)*4*1, PROT_READ | PROT_WRITE, MAP_SHARED, eyeFd, 0);
    if (eyPtr == MAP_FAILED) {
        cout << "SOMETHING'S FUCKY: eyPtr mmap failed" << endl;
    }

    while (1);
}
