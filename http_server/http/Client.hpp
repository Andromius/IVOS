#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "Response.hpp"
#include "Request.hpp"
#include "../logging/Logger.hpp"
#include "../logging/LogMessage.hpp"

#include <memory>
#include <unistd.h>
#include <strings.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <mqueue.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <wait.h>

class Client
{
private:
    std::map<std::string, std::string> _runners = {
        {"py", "python3"},
        {"sh", "bash"},
        {"php", "php"}
    };

    char _remainder[1024];
    size_t _remaining_size = 0;

    bool _is_open;
    bool _peer_disconnected = false;
    int _socket;
    char _buffer[1024] = { 0 };
    ssize_t _valread;
    mqd_t _message_queue;
    SSL* _ssl;
    
    struct sockaddr_in _addr;
    socklen_t _addr_len = sizeof(_addr);

public:
    Client(int socket, mqd_t message_queue, SSL_CTX* ssl_context);
    ~Client();
    
    void send(Response& response);
    bool receive_headers(Request& request);

    void handle_post(Request& request);
    std::string get_ip();
    std::string get_port();
    void execute_script(Request& request);
    
    void close();
    bool is_open();


};

#endif // CLIENT_HPP
