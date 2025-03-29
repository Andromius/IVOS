#ifndef COMPOSITE_LOGGER_HPP
#define COMPOSITE_LOGGER_HPP

#include "Logger.hpp"
#include <vector>

class CompositeLogger : public Logger
{
private:
    std::vector<Logger*> _loggers;
public:
    ~CompositeLogger();

    void log(LogLevel level, std::time_t time, std::string message) override;
    void add_logger(Logger* logger);
};

#endif // COMPOSITE_LOGGER_HPP
