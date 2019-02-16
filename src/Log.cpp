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
    : logTo(LogTo::nowhere)
    , lout(&std::cout)
    , lerr(&std::cerr)
    , expandedLogFilePath("")
    , logName("noname")
    , autoLogRotationEnabled(false)
    , rotateLogs(false)
{
}


Log::~Log()
{
    stopLogging();
}


bool Log::setAutoLogRotation(unsigned int intervalMinutes, int offsetHour, int offsetMinute)
{
    autoLogRotationEnabled = false; 

    if (logTo != LogTo::file && logTo != LogTo::fileWithTimestamp
        && logTo != LogTo::redirect && logTo != LogTo::redirectWithTimestamp)
    {
        logMsg(LOG_ERR, "Log rotation is supported only when logging to a file.");
        return false;
    }

    if (offsetHour < 0 || offsetHour > 23) {
        logMsg(LOG_ERR, "Invalid log rotation offsetHour %d.  Valid values are 0 - 23.", offsetHour);
        return false;
    }
    if (offsetMinute < 0 || offsetMinute > 59) {
        logMsg(LOG_ERR, "Invalid log rotation offsetMinute %d.  Valid values are 0 - 59.", offsetMinute);
        return false;
    }
    if (intervalMinutes < 1) {
        logMsg(LOG_ERR, "Invalid log rotation intervalMinutes %d.  Valid values are greater than zero.", intervalMinutes);
        return false;
    }
    if (intervalMinutes < 60) {
        logMsg(LOG_WARNING, "Log rotation intervalMinutes is %d, which seems like a very short interval.", intervalMinutes);
    }

    logRotationIntervalSeconds = intervalMinutes * 60;

    // TODO:  explain how we're using the offset to come up with the next rotation time
    time_t now = getNowSeconds();
    struct tm result;
    struct tm tmOffset = *localtime_r(&now, &result);
    tmOffset.tm_sec = 0;
    tmOffset.tm_min = offsetMinute;
    tmOffset.tm_hour = offsetHour;
    nextLogRotationTime = mktime(&tmOffset);
    while (nextLogRotationTime <= now) {
        nextLogRotationTime += logRotationIntervalSeconds;
    }

    autoLogRotationEnabled = true; 

    return true;
}


std::string Log::doRotateLogs()
{
    if (logTo == LogTo::file || logTo == LogTo::fileWithTimestamp) {
        closeLogFile();
    }
    else {
        closeRedirectionLogFile();
    }

    // If the log file was created without a timestamp in its name, add a
    // timestamp indicating the end of the period covered by the file (now).
    std::string rotatedLogFilePathName;
    if (logTo == LogTo::file || logTo == LogTo::redirect) {
        rotatedLogFilePathName =
            expandedLogFilePath + logName + "_" + getTimestamp(TimestampType::compactYmdHm) + ".log";
        if (rename(logFilePathName.c_str(), rotatedLogFilePathName.c_str()) != 0) {
            int errNum = errno;
            std::cerr << std::string(__FUNCTION__)
                << ":  Unable to rename rotated log file " << logFilePathName << " to " << rotatedLogFilePathName << "; "
                << std::string(strerror(errNum)) + " (" + std::to_string(errNum) + ")." << std::endl;
            // We'll try to keep going with the old name, essentially not rotating the log file.
            rotatedLogFilePathName = "*** file rename error ***";
        }
    }
    // Otherwise, the active log file is supposed to have a timestamp
    // in its name, so update the timestamp to the current time.
    else {
        rotatedLogFilePathName = logFilePathName;
        resolveLogFilePathName();
    }

    if (logTo == LogTo::file || logTo == LogTo::fileWithTimestamp) {
        if (!openLogFile()) {
            // Not much else we can do other than make no more logging attempts.
            logTo = LogTo::nowhere;
        }
    }
    else {
        if (!openRedirectionLogFile()) {
            // Not much else we can do other than make no more logging attempts.
            logTo = LogTo::nowhere;
        }
    }

    return rotatedLogFilePathName;
}


std::string Log::getTimestamp(TimestampType timestampType)
{
    uint64_t nowMs = getNowMs64();
    time_t now = nowMs / 1000;
    struct tm result;
    struct tm tmNow = *localtime_r(&now, &result);

    char buf[20];
    uint32_t ms;
    std::stringstream sstr;
    std::string timestamp;

    switch (timestampType) {

        case TimestampType::delimitedYmdHmsn:
            std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tmNow);
            ms = nowMs % 1000;
            sstr << buf << "." << std::setfill('0') << std::setw(3) << ms;
            timestamp = sstr.str();
            break;

        case TimestampType::compactYmdHm:
            std::strftime(buf, sizeof(buf), "%Y%m%d_%H%M", &tmNow);
            timestamp = buf;
            break;
    }

    // This is a convenient place to check if we need to rotate the logs
    // because we've already gone through the trouble of getting the time.
    if (autoLogRotationEnabled && now >= nextLogRotationTime) {
        rotateLogs = true;
        // We might need to advance the next rotation time by multiple intervals
        // because there could be a gap between log entries that is longer than
        // one interval.
        while (nextLogRotationTime <= now) {
            nextLogRotationTime += logRotationIntervalSeconds;
        }
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
        return;
    }

    // getTimestamp also checks if it is time to rotate the logs.  Messy but efficient.
    std::string timestamp = getTimestamp(TimestampType::delimitedYmdHmsn);

    if (rotateLogs) {
        rotateLogs = false;
        *lout << timestamp << ":  Rotating log file." << std::endl;
        std::string rotatedLogFilePathName = doRotateLogs();
        *lout << timestamp << ":  Rotated log file.  Previous log file is " << rotatedLogFilePathName << std::endl;
    }

    vsnprintf(sbuf, sizeof(sbuf), format, args);

    if (priority > LOG_WARNING) {
        *lout << timestamp << ":  " << sbuf << std::endl;
    }
    else if (priority == LOG_WARNING) {
        *lerr << timestamp << ":  ///// Warning:  " << sbuf << " /////" << std::endl;
    }
    else {
        *lerr << timestamp << ":  *** " << sbuf << std::endl;
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


bool Log::openLogFile()
{
    bool successful = false;

    flog.open(logFilePathName.c_str(), std::ios_base::out | std::ios_base::app);
    if (flog.is_open()) {
        lout = lerr = &flog;
        successful = true;
    }
    else {
        int errNum = errno;
        std::cerr << std::string(__FUNCTION__)
            << ":  Unable to open output file " << logFilePathName << "; "
            << std::string(strerror(errNum)) + " (" + std::to_string(errNum) + ")." << std::endl;
    }

    return successful;
}


void Log::closeLogFile()
{
    if (flog.is_open()) {
        flog.close();
    }
}


bool Log::openRedirectionLogFile()
{
    bool successful = false;

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
            int errNum = errno;
            // We should have more granular error detection so that we can close the
            // right descriptors, but the world is probably fucked anyway so who cares.
            close(redirectionFd);
            std::cerr << std::string(__FUNCTION__)
                << ":  Unable to save or duplicate file descriptors; "
                << std::string(strerror(errNum)) + " (" + std::to_string(errNum) + ")." << std::endl;
        }
    }
    else {
        int errNum = errno;
        std::cerr << std::string(__FUNCTION__)
            << ":  Unable to open redirection output file " << logFilePathName << "; "
            << std::string(strerror(errNum)) + " (" + std::to_string(errNum) + ")." << std::endl;
    }

    return successful;
}


void Log::closeRedirectionLogFile()
{
    fflush(stdout);
    fflush(stderr);
    close(redirectionFd);
    dup2(saveStdoutFd, fileno(stdout));
    close(saveStdoutFd);
    dup2(saveStderrFd, fileno(stderr));
    close(saveStderrFd);
}


bool Log::startLogging(const std::string& logName, LogTo logTo, const std::string& logFilePath)
{
    stopLogging();

    this->logName = logName;
    this->logTo = logTo;

    bool successful = false;
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
                successful = openLogFile();
            }
            break;

        case LogTo::redirect:
        case LogTo::redirectWithTimestamp:
            if (resolveLogFilePathName(logFilePath)) {
                successful = openRedirectionLogFile();
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
            closeLogFile();
            break;

        case LogTo::redirect:
        case LogTo::redirectWithTimestamp:
            closeRedirectionLogFile();
            break;
    }

    logTo = LogTo::nowhere;

    // We shouldn't attempt to write anything anywhere, but make
    // sure that lout and lerr are valid in case an attempt is made.
    lout = &std::cout;
    lerr = &std::cerr;
}

