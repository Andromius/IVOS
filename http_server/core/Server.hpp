#ifndef SERVER_HPP
#define SERVER_HPP

#include "../logging/CompositeLogger.hpp"
#include "../http/Client.hpp"

#include <netinet/in.h>
#include <signal.h>
#include <semaphore.h>
#include <openssl/ssl.h>
#include <queue>
#include <mutex>
#include <mqueue.h>
#include <errno.h>
#include <string.h>
#include <cstdio>
#include <array>

class Server
{
private:
    static bool _should_stop;
    static Server* _instance;
    static void cancellation_requested(int sig_num);
    
    uint16_t _port = 8080; 
    int _max_connections = 10;

    SSL_CTX* _ssl_context;
    int _server_fd;
    struct sockaddr_in _address;
    int _opt = 1;
    socklen_t addrlen = sizeof(_address);

    mqd_t _message_queue;
    
    pid_t _logger_pid;
    CompositeLogger* _logger;

    std::vector<pid_t> _client_processes;
    std::vector<std::array<int, 2>> _socket_pairs;

    std::mutex* _available_clients_mutex;
    mqd_t _available_clients;

    int send_fd(int socket, int fd);
    int recv_fd(int socket);
    
    void handle_communication(int receiving_socket);
    void start_logger_process();
    void start_client_processes();
    void set_up_socket();
protected:
    Server();

public:
    ~Server();
    static Server* get_instance();
    
    Server(Server &other) = delete;

    void operator=(const Server &) = delete;

    Server* configure_max_connections(int max_connections);
    Server* configure_port(uint16_t port);
    Server* configure_ssl(std::string certificate_path, std::string private_key_path);
    
    Server* add_console_logger();
    Server* add_logger(std::string file_path);

    void run();
};

#endif // SERVER_HPP
