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

#include <cstdint>
#include <iostream>
#include <iomanip>
#include <sstream>

#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>

#include "illumiconeUtility.h"
#include "log.h"


Log::Log()
    : useSystemLog(false)
{
}


Log::~Log()
{
}


const std::string Log::getLogMsgTimestamp()
{
    uint64_t nowMs = getNowMs64();
    uint32_t ms = nowMs % 1000;
    time_t now = nowMs / 1000;

    struct tm result;
    struct tm tmStruct = *localtime_r(&now, &result);
    char buf[20];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tmStruct);

    std::stringstream sstr;
    sstr << buf << "." << std::setfill('0') << std::setw(3) << ms << ":  ";

    std::string str = sstr.str();
    return str;
}


void Log::logMsg(int priority, const std::string& message)
{
    logMsg(priority, "%s", message.c_str());
}


void Log::logMsg(int priority, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    vlogMsg(priority, format, args);
    va_end(args);
}


void Log::logMsg(int priority, int errNum, const std::string& message)
{
    logMsg(priority, errNum, "%s", message.c_str());
}


void Log::logMsg(int priority, int errNum, const char* format, ...)
{
    std::string errNumStr = "  " + std::string(strerror(errNum)) + " (" + std::to_string(errNum) + ")";

    std::string formatWithErrNumStr = format + errNumStr;

    va_list args;
    va_start(args, format);
    vlogMsg(priority, formatWithErrNumStr.c_str(), args);
    va_end(args);
}


void Log::vlogMsg(int priority, const char* format, va_list args)
{
    if (useSystemLog) {
        vsyslog(priority, format, args);
    }
    else {
        char buf[stringBufSize];
        vsnprintf(buf, sizeof(buf), format, args);

        if (priority > LOG_WARNING) {
            std::cout << getLogMsgTimestamp() << buf << std::endl;
        }
        else if (priority == LOG_WARNING) {
            std::cerr << getLogMsgTimestamp() << "///// Warning:  " << buf << " /////" << std::endl;
        }
        else {
            std::cerr << getLogMsgTimestamp() << "*** " << buf << std::endl;
        }
    }
}


void Log::startLogging(bool useSystemLog, const char* appName)
{
    this.useSystemLog = useSystemLog;

    if (useSystemLog) {
        openlog(appName, LOG_PID | LOG_CONS, LOG_USER);
    }
}


void Log::stopLogging()
{
    if (useSystemLog) {
        closelog();
    }
}



