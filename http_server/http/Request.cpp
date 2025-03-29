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
    return _string_to_type.at(input);
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

    if (_fields.count("Content-Type") != 0)
    {
        tokens = string_split(_fields["Content-Type"], "; ");
        for (auto &&token : tokens)
        {
            if (token.find("boundary") != std::string::npos)
            {
                _boundary = string_split(token, "=")[1];
            }
        }
    }
    
    return;
}

RequestType Request::get_type()
{
    return _type;
}

std::string Request::get_type_as_string()
{
    return _type_to_string.at(_type);
}

bool Request::has_path()
{
    return _path.length() > 1;
}

std::string Request::get_path()
{
    return _path;
}

bool Request::keep_alive()
{
    if (_fields.find("Connection") != _fields.end())
        return _fields["Connection"] == "keep-alive";
    return false;
}

std::string Request::get_path_extension()
{
    std::string extension = _path.substr(_path.find_last_of(".") + 1);
    return extension;
}

bool Request::is_script()
{
    std::string extension = get_path_extension();
    if (extension == "py" || extension == "sh" || extension == "php")
        return true;
    return false;
}

std::string Request::get_boundary()
{
    return _boundary;
}

std::string Request::get_field(std::string field)
{
    if (_fields.count(field) <= 0)
    {
        return "";
    }
    return _fields[field];
}
