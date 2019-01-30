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

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>
#include <wordexp.h>

#include "illumiconeUtility.h"
#include "Log.h"


Log::Log()
    : lout(&std::cout)
    , lerr(&std::cerr)
    , logName("noname")
    , logTo(LogTo::nowhere)
{
}


Log::~Log()
{
    stopLogging();
}


const std::string Log::getTimestamp(TimestampType timestampType)
{
    uint64_t nowMs = getNowMs64();
    time_t now = nowMs / 1000;
    struct tm result;
    struct tm tmStruct = *localtime_r(&now, &result);

    char buf[20];
    uint32_t ms;
    std::stringstream sstr;
    std::string timestamp;

    switch (timestampType) {

        case TimestampType::delimitedYmdHmsn:
            std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tmStruct);
            ms = nowMs % 1000;
            sstr << buf << "." << std::setfill('0') << std::setw(3) << ms;
            timestamp = sstr.str();
            break;

        case TimestampType::compactYmdHm:
            std::strftime(buf, sizeof(buf), "%Y%m%d_%H%M", &tmStruct);
            timestamp = buf;
            break;
    }

    return timestamp;
}


void Log::logMsg(int priority, const std::string& message)
{
    if (logTo == LogTo::nowhere) return;

    logMsg(priority, "%s", message.c_str());
}


void Log::logMsg(int priority, const char* format, ...)
{
    if (logTo == LogTo::nowhere) return;

    va_list args;
    va_start(args, format);
    vlogMsg(priority, format, args);
    va_end(args);
}


void Log::logMsg(int priority, int errNum, const std::string& message)
{
    if (logTo == LogTo::nowhere) return;

    logMsg(priority, errNum, "%s", message.c_str());
}


void Log::logMsg(int priority, int errNum, const char* format, ...)
{
    if (logTo == LogTo::nowhere) return;

    std::string errNumStr = "  " + std::string(strerror(errNum)) + " (" + std::to_string(errNum) + ")";

    std::string formatWithErrNumStr = format + errNumStr;

    va_list args;
    va_start(args, format);
    vlogMsg(priority, formatWithErrNumStr.c_str(), args);
    va_end(args);
}


void Log::vlogMsg(int priority, const char* format, va_list args)
{
    if (logTo == LogTo::nowhere) return;

    if (logTo == LogTo::systemLog) {
        vsyslog(priority, format, args);
    }
    else {
        vsnprintf(sbuf, sizeof(sbuf), format, args);

        if (priority > LOG_WARNING) {
            *lout << getTimestamp(TimestampType::delimitedYmdHmsn) << ":  " << sbuf << std::endl;
        }
        else if (priority == LOG_WARNING) {
            *lerr << getTimestamp(TimestampType::delimitedYmdHmsn) << ":  ///// Warning:  " << sbuf << " /////" << std::endl;
        }
        else {
            *lerr << getTimestamp(TimestampType::delimitedYmdHmsn) << ":  *** " << sbuf << std::endl;
        }
    }
}


bool Log::resolveLogFilePathName(const std::string& logFilePath)
{
    if (!logFilePath.empty()) {
        // Expand the tilde and any environment variables the path might contain.
        wordexp_t p;
        if (wordexp(logFilePath.c_str(), &p, WRDE_NOCMD | WRDE_UNDEF) != 0 || p.we_wordc != 1) {
            std::cerr << std::string(__FUNCTION__)
                << ":  Invalid log file path \"" << logFilePath << "\"." << std::endl;
            return false;
        }
        expandedLogFilePath = p.we_wordv[0];
        wordfree(&p);

        // We expect the path to end in a slash.
        if (expandedLogFilePath.back() != '/') {
            expandedLogFilePath += "/";
        }
    }

    if (logTo == LogTo::fileWithTimestamp || logTo == LogTo::redirectWithTimestamp) {
        logFilePathName = expandedLogFilePath + logName + "_" + getTimestamp(TimestampType::compactYmdHm) + ".log";
    }
    else {
        logFilePathName = expandedLogFilePath + logName + ".log";
    }

    return true;
}


bool Log::startLogging(const std::string& logName, LogTo logTo, const std::string& logFilePath)
{
    stopLogging();

    this->logName = logName;
    this->logTo = logTo;

    bool successful = false;
    int errNum;
    switch (this->logTo) {

        case LogTo::nowhere:
        case LogTo::console:
            lout = &std::cout;
            lerr = &std::cerr;
            successful = true;
            break;

        case LogTo::systemLog:
            openlog(this->logName.c_str(), LOG_PID | LOG_CONS | LOG_NDELAY, LOG_USER);
            successful = true;
            break;

        case LogTo::file:
        case LogTo::fileWithTimestamp:
            if (resolveLogFilePathName(logFilePath)) {
                flog.open(logFilePathName.c_str(), std::ios_base::out | std::ios_base::app);
                if (flog.is_open()) {
                    lout = lerr = &flog;
                    successful = true;
                }
                else {
                    errNum = errno;
                    std::cerr << std::string(__FUNCTION__)
                        << ":  Unable to open output file " << logFilePathName << "; "
                        << std::string(strerror(errNum)) + " (" + std::to_string(errNum) + ")." << std::endl;
                }
            }
            break;

        case LogTo::redirect:
        case LogTo::redirectWithTimestamp:
            if (resolveLogFilePathName(logFilePath)) {
                redirectionFd = open(logFilePathName.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
                if (redirectionFd != -1) {
                    saveStdoutFd = dup(fileno(stdout));
                    saveStderrFd = dup(fileno(stderr));
                    if (saveStdoutFd != -1
                        && saveStderrFd != -1
                        && dup2(redirectionFd, fileno(stdout)) != -1
                        && dup2(redirectionFd, fileno(stderr)) != -1)
                    {
                        lout = &std::cout;
                        lerr = &std::cerr;
                        successful = true;
                    }
                    else {
                        errNum = errno;
                        // We should have more granular error detection so that we can close the
                        // right descriptors, but the world is probably fucked anyway so who cares.
                        close(redirectionFd);
                        std::cerr << std::string(__FUNCTION__)
                            << ":  Unable to save or duplicate file descriptors; "
                            << std::string(strerror(errNum)) + " (" + std::to_string(errNum) + ")." << std::endl;
                    }
                }
                else {
                    errNum = errno;
                    std::cerr << std::string(__FUNCTION__)
                        << ":  Unable to open redirection output file " << logFilePathName << "; "
                        << std::string(strerror(errNum)) + " (" + std::to_string(errNum) + ")." << std::endl;
                }
            }
            break;
    }

    if (!successful) {
        this->logTo = LogTo::nowhere;
    }

    return successful;
}


void Log::stopLogging()
{
    switch (this->logTo) {

        case LogTo::nowhere:
        case LogTo::console:
            break;

        case LogTo::systemLog:
            closelog();
            break;

        case LogTo::file:
        case LogTo::fileWithTimestamp:
            if (flog.is_open()) {
                flog.close();
            }
            break;

        case LogTo::redirect:
        case LogTo::redirectWithTimestamp:
            fflush(stdout);
            fflush(stderr);
            close(redirectionFd);
            dup2(saveStdoutFd, fileno(stdout));
            close(saveStdoutFd);
            dup2(saveStderrFd, fileno(stderr));
            close(saveStderrFd);
            break;
    }

    logTo = LogTo::nowhere;

    // We shouldn't attempt to write anything anywhere, but make
    // sure that lout and lerr are valid in case an attempt is made.
    lout = &std::cout;
    lerr = &std::cerr;
}

