#include "Logger.hpp"

Logger::Logger()
{
    _fd = fileno(stdout);
}

Logger::Logger(int fd)
{
    _fd = fd;
}

Logger::~Logger()
{
}

void Logger::log(LogLevel level, std::string message)
{
    _mutex.lock();
    dprintf(_fd, "[%s] %s\n", _levelStrings[level].c_str(), message.c_str());
    _mutex.unlock();
}
