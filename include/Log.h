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

#pragma once


#include <string>

#include <stdarg.h>
#include <syslog.h>


class Log
{
    public:

        Log();
        virtual ~Log();

        Log(const Log&) = delete;
        Log& operator =(const Log&) = delete;

        void logMsg(int priority, const std::string& message);
        void logMsg(int priority, const char* format, ...) __attribute__ ((format (printf, 3, 4)));
        void logMsg(int priority, int errNum, const std::string& message);
        void logMsg(int priority, int errNum, const char* format, ...) __attribute__ ((format (printf, 4, 5)));
        void vlogMsg(int priority, const char* format, va_list args);

        void startLogging(bool useSystemLog, const char* appName);
        void stopLogging();

    private:

        constexpr static unsigned int stringBufSize = 1024;

        bool useSystemLog;

        const std::string getLogMsgTimestamp();

};

