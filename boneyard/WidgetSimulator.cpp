/*
    This file is part of Illumicone.

    Illumicone is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Illumicone is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Illumicone.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <iostream>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cstdbool>
#include <unistd.h>
#include <sys/types.h>

using namespace std;

int main()
{
    int threeWheelFd;
    int eyeFd;
    void *twPtr;
    void *eyPtr;

    int prevPositionR = 0;
    int positionR = 0;
    int velocityR = 0;

    int prevPositionG = 0;
    int positionG = 0;
    int velocityG = 0;

    int prevPositionB = 0;
    int positionB = 0;
    int velocityB = 0;

    threeWheelFd = shm_open("/threeWheelArea", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    eyeFd = shm_open("/eyeArea", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);

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

    while (1) {
        
    }
}
