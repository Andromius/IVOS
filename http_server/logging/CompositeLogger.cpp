#include "CompositeLogger.hpp"

CompositeLogger::~CompositeLogger()
{
    for (auto logger : _loggers)
    {
        delete logger;
    }

    _loggers.clear();
}

void CompositeLogger::log(LogLevel level, std::time_t time, std::string message)
{
    for (auto logger : _loggers)
    {
        logger->log(level, time, message);
    }
}

void CompositeLogger::add_logger(Logger *logger)
{
    _loggers.push_back(logger);
}
