#include "Logger.hpp"

Logger::Logger()
{
    _out_stream.open("/dev/stdout");
}

Logger::Logger(std::string file_path)
{
    _out_stream.open(file_path, std::ios::trunc | std::ios::out);
    if (!_out_stream.is_open())
    {
        perror("Failed to open log file");
        exit(EXIT_FAILURE);
    }
}

void Logger::log(LogLevel level, std::time_t time, std::string message)
{
    std::tm* local_time = std::localtime(&time);
    std::ostringstream out_stream;
    _out_stream << "[" << std::put_time(local_time, "%Y-%m-%d %H:%M:%S") << "] [" << _levelStrings[level] << "] " << message << std::endl;
    _out_stream.flush();
}
