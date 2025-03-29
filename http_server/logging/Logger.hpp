#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <iostream>
#include <string>
#include <map>
#include <ctime>
#include <iomanip>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <fstream>
#include "LogLevel.hpp"

class Logger
{
private:
    std::map<LogLevel, std::string> _levelStrings = {
        {Debug, "DEBUG"},
        {Info, "INFO"},
        {Warning, "WARNING"},
        {Error, "ERROR"},
        {Fatal, "FATAL"}
    };
    std::ofstream _out_stream;

public:
    Logger();
    Logger(std::string file_path);

    virtual void log(LogLevel level, std::time_t time, std::string message);
};

#endif // LOGGER_HPP
