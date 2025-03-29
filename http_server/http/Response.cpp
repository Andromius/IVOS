#include "Response.hpp"

bool Response::load_file(std::string path)
{
    std::ifstream t(_default_directory + path);
    if (!t.is_open())
        return false;

    t.seekg(0, std::ios::end);
    _content_length = t.tellg();
    
    _content.reserve(_content_length);
    t.seekg(0, std::ios::beg);
    _content.assign((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
    
    t.close();

    return true;
}

void Response::set_mime_type(std::string path)
{
    size_t dot_pos = path.find_last_of('.');
    if (dot_pos == std::string::npos) {
        _mime_type = "text/html";
        return;
    }

    if (_mime_types.find(path.substr(dot_pos)) == _mime_types.end())
    {
        _mime_type = "text/html";
        return;
    }

    _mime_type = _mime_types.at(path.substr(dot_pos));
}

Response::Response()
{
}

Response::~Response()
{
}

std::string Response::get_response()
{
    return _response;
}

void Response::create_string_response(std::string response)
{
    //std::cout << response.length() << std::endl;
    _response = _version + " 200 OK";
    _response = _response.append("\r\nContent-Length: ");
    _response = _response.append(std::to_string(response.length()));
    _response = _response.append("\r\nContent-Type: ");
    _response = _response.append(_mime_type);
    _response = _response.append("\r\n\r\n");
    _response = _response.append(response);
}

void Response::create_file_response(std::string path)
{
    if (!load_file(path))
    {
        create_not_found_response();
        return;
    }

    set_mime_type(path);
    
    _response = _version + " 200 OK";
    _response = _response.append("\r\nContent-Length: ");
    _response = _response.append(std::to_string(_content_length));
    _response = _response.append("\r\nContent-Type: ");
    _response = _response.append(_mime_type);
    _response = _response.append("\r\n\r\n");
    _response = _response.append(_content);
}

void Response::create_not_found_response()
{
    load_file("/not_found.html");
    set_mime_type("/not_found.html");

    _response = _version + " 404 Not Found";
    _response = _response.append("\r\nContent-Length: ");
    _response = _response.append(std::to_string(_content_length)); 
    _response = _response.append("\r\nContent-Type: ");
    _response = _response.append(_mime_type);
    _response = _response.append("\r\n\r\n");
    _response = _response.append(_content);
}

void Response::create_service_unavailable_response()
{
    load_file("/service_unavailable.html");
    set_mime_type("/service_unavailable.html");

    _response = _version + " 503 Service Unavailable";
    _response = _response.append("\r\nContent-Length: ");
    _response = _response.append(std::to_string(_content_length));
    _response = _response.append("\r\nContent-Type: ");
    _response = _response.append(_mime_type);
    _response = _response.append("\r\n\r\n");
    _response = _response.append(_content);
}

int Response::get_response_length()
{
    return _response.length();
}

void Response::create_created_response()
{
    std::string response = "received";
    _response = _version + " 201 CREATED";
    _response = _response.append("\r\nContent-Length: ");
    _response = _response.append(std::to_string(response.length()));
    _response = _response.append("\r\nContent-Type: ");
    _response = _response.append(_mime_type);
    _response = _response.append("\r\n\r\n");
    _response = _response.append(response);
}
