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
#include "Log.h"

using namespace std;


extern Log logger;


int acquireProcessLock(const string& lockFilePath, bool logIfLocked)
{
    int fd = open(lockFilePath.c_str(), O_CREAT, S_IRUSR | S_IWUSR);
    if (fd >= 0) {
        if (flock(fd, LOCK_EX | LOCK_NB) == 0) {
            if (logIfLocked) {
                logger.logMsg(LOG_INFO, "Acquired lock on " + lockFilePath + ".");
            }
            return fd;
        }
        else {
            if (errno == EWOULDBLOCK) {
                close(fd);
                if (logIfLocked) {
                    logger.logMsg(LOG_INFO, "Another process has locked " + lockFilePath + ".");
                }
                return -1;
            }
            else {
                close(fd);
                logger.logMsg(LOG_ERR, errno, "Unable to lock " + lockFilePath + ".");
                return -1;
            }
        }
    }
    else {
        logger.logMsg(LOG_ERR, errno, "Unable to create or open " + lockFilePath + ".");
        return -1;
    }
}


// This function is used by the beat generators in FastLED's lib8tion.
uint32_t get_millisecond_timer()
{
    return (uint32_t) getNowMs64();
}


uint32_t getNowMs()
{
    return (uint32_t) getNowMs64();
}


uint64_t getNowMs64()
{
    using namespace std::chrono;

    milliseconds epochMs = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
    return epochMs.count();
}


uint32_t getNowSeconds()
{
    return (uint32_t) getNowMs64() / 1000;
}


