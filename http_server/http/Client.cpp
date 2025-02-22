#include "Client.hpp"

Client::Client(int socket)
{
    _socket = socket;
}

void Client::send(Response& response)
{
    ::send(_socket, response.get_response().c_str(), response.get_response_length(), 0);
}

Request Client::receive()
{
    _valread = read(_socket, _buffer, 1024 - 1); // subtract 1 for the null terminator at the end
    if (_valread == -1)
    {
        perror("read");
        exit(EXIT_FAILURE);
    }
    
    printf("%s\n", _buffer);
    Request request = Request();
    request.parse(_buffer);
    return request;
}

void Client::close()
{
    ::close(_socket);
}
