#include "Request.hpp"

std::vector<std::string> Request::string_split(std::string& input, std::string delimiter)
{
    std::vector<std::string> tokens;
    size_t position;
    std::string token;
    while ((position = input.find(delimiter)) != std::string::npos) {
        token = input.substr(0, position);
        token.erase(std::remove_if(token.begin(), token.end(), iscntrl), token.end());
        tokens.push_back(token);
        input.erase(0, position + delimiter.length());

    }
    tokens.push_back(input);
    return tokens;
}

RequestType Request::to_request_type(std::string &input)
{
    return _mapping.at(input);
}

Request::Request()
{
    _fields = std::map<std::string, std::string>();
}

Request::~Request()
{
}

void Request::parse(std::string request)
{
    std::vector<std::string> lines = string_split(request, "\n");
    if (lines.size() == 0) 
        return;

    std::vector<std::string> tokens = string_split(lines[0], " ");
    _type = to_request_type(tokens[0]);
    _path = tokens[1];
    _version = tokens[2];

    for (size_t i = 1; i < lines.size(); i++)
    {
        tokens = string_split(lines[i], ": ");
        if (tokens.size() == 1) 
            continue;
        _fields[tokens[0]] = tokens[1];
    }
    
    return;
}

RequestType Request::get_type()
{
    return _type;
}

bool Request::has_path()
{
    return _path.length() > 1;
}

std::string Request::get_path()
{
    return _path;
}
