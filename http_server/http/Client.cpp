#include "Client.hpp"

Client::Client(int socket)
{
    _socket = socket;
    _is_open = true;
}

void Client::send(Response& response)
{
    if (!_is_open)
        return;
    ::send(_socket, response.get_response().c_str(), response.get_response_length(), 0);
}

bool Client::receive(Request& request)
{
    bzero(_buffer, 1024);
    _valread = read(_socket, _buffer, 1024 - 1); // subtract 1 for the null terminator at the end
    if (_valread == -1)
    {
        perror("read");
        close();
        return false;
    }

    if (_valread == 0)
    {
        close();
        return false;
    }

    printf("%s\n", _buffer);
    request.parse(_buffer);
    return true;
}

void Client::close()
{
    ::close(_socket);
    _is_open = false;
}

bool Client::is_open()
{
    return _is_open;
}
