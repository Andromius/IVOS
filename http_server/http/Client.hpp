#include "Response.hpp"
#include "Request.hpp"

#include <sys/socket.h>
#include <unistd.h>
#include <strings.h>

class Client
{
private:
    bool _is_open;
    int _socket;
    char _buffer[1024] = { 0 };
    ssize_t _valread;
public:
    Client(int socket);
    
    void send(Response& response);
    bool receive(Request& request);
    
    void close();
    bool is_open();

};
