#include <iostream>
#include <string>
#include <map>
#include <mutex>
#include "LogLevel.hpp"


class Logger
{
private:
    int _fd;
    std::map<LogLevel, std::string> _levelStrings = {
        {Debug, "DEBUG"},
        {Info, "INFO"},
        {Warning, "WARNING"},
        {Error, "ERROR"},
        {Fatal, "FATAL"}
    };
    std::mutex _mutex;

public:
    Logger();
    Logger(int fd);
    ~Logger();

    void log(LogLevel level, std::string message);
};
