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

#include <chrono>

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/file.h>
#include <sys/types.h>
#include <unistd.h>

#include "illumiconeUtility.h"


using namespace std;


int acquireProcessLock(const string& lockFilePath)
{
    int fd = open(lockFilePath.c_str(), O_CREAT, S_IRUSR | S_IWUSR);
    if (fd >= 0) {
        if (flock(fd, LOCK_EX | LOCK_NB) == 0) {
            return fd;
        }
        else {
            if (errno == EWOULDBLOCK) {
                close(fd);
                fprintf(stderr, "Another process has locked %s.\n", lockFilePath.c_str());
                return -1;
            }
            else {
                close(fd);
                fprintf(stderr, "Unable to lock %s.  Error %d:  %s\n", lockFilePath.c_str(), errno, strerror(errno));
                return -1;
            }
        }
    }
    else {
        fprintf(stderr, "Unable to create or open %s.  Error %d:  %s\n", lockFilePath.c_str(), errno, strerror(errno));
        return -1;
    }
}


unsigned int getNowMs()
{
    using namespace std::chrono;

    milliseconds epochMs = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
    unsigned int nowMs = epochMs.count();

    return nowMs;
}


// This function is used by the beat generators in FastLED's lib8tion.
uint32_t get_millisecond_timer()
{
    return getNowMs();
}


