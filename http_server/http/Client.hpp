#include "Response.hpp"
#include "Request.hpp"

#include <sys/socket.h>
#include <unistd.h>

class Client
{
private:
    int _socket;
    char _buffer[1024] = { 0 };
    ssize_t _valread;
public:
    Client(int socket);
    
    void send(Response& response);
    Request receive();
    
    void close();

};
