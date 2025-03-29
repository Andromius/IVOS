#pragma once

#include "LogLevel.hpp"

#include <ctime>
#include <string>
#include <cstring>


namespace messaging
{
    struct LogMessage
    {
        LogLevel _level;
        std::time_t _time;
        char _message[256];
    };
    
    inline LogMessage createLogMessage(LogLevel level, const std::string& msg)
    {
        LogMessage logMsg;
        logMsg._level = level;
        logMsg._time = std::time(nullptr);
        
        std::strncpy(logMsg._message, msg.c_str(), sizeof(logMsg._message) - 1);
        logMsg._message[sizeof(logMsg._message) - 1] = '\0';

        return logMsg;
    }
}