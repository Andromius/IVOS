#include "Server.hpp"

Server* Server::_instance = nullptr;
bool Server::_should_stop = false;

void Server::cancellation_requested(int sig_num)
{
    Server::_should_stop = true;
}

int Server::send_fd(int socket, int fd) {
    struct msghdr msg = {0};
    struct iovec iov;
    char buf[1] = {0};
    struct cmsghdr *cmsg;
    char cmsgbuf[CMSG_SPACE(sizeof(fd))];

    iov.iov_base = buf;
    iov.iov_len = sizeof(buf);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    msg.msg_control = cmsgbuf;
    msg.msg_controllen = sizeof(cmsgbuf);

    cmsg = CMSG_FIRSTHDR(&msg);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    cmsg->cmsg_len = CMSG_LEN(sizeof(fd));

    memcpy(CMSG_DATA(cmsg), &fd, sizeof(fd));

    messaging::LogMessage message;
    if (sendmsg(socket, &msg, 0) < 0) 
    {
        message = messaging::createLogMessage(LogLevel::Error, "sendmsg: " + std::string(strerror(errno)));
        mq_send(_message_queue, (char*)&message, sizeof(messaging::LogMessage), 0);
        return -1;
    }

    return 0;
}

int Server::recv_fd(int socket) 
{
    struct msghdr msg = {0};
    struct iovec iov;
    char buf[1];
    struct cmsghdr *cmsg;
    char cmsgbuf[CMSG_SPACE(sizeof(int))];
    int fd;

    iov.iov_base = buf;
    iov.iov_len = sizeof(buf);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    msg.msg_control = cmsgbuf;
    msg.msg_controllen = sizeof(cmsgbuf);

    messaging::LogMessage message;
    if (recvmsg(socket, &msg, 0) < 0) 
    {
        message = messaging::createLogMessage(LogLevel::Error, "recvmsg: " + std::string(strerror(errno)));
        mq_send(_message_queue, (char*)&message, sizeof(messaging::LogMessage), 0);
        return -1;
    }

    cmsg = CMSG_FIRSTHDR(&msg);
    if (cmsg == NULL) 
    {
        message = messaging::createLogMessage(LogLevel::Error, "No control message received");
        mq_send(_message_queue, (char*)&message, sizeof(messaging::LogMessage), 0);
        return -1;
    }

    memcpy(&fd, CMSG_DATA(cmsg), sizeof(fd));

    return fd;
}

void Server::handle_communication(int receiving_socket)
{
    messaging::LogMessage message;
    while (!Server::_should_stop)
    {
        message = messaging::createLogMessage(LogLevel::Debug, "PID: " + std::to_string(getpid()) + " waiting for connection");
        mq_send(_message_queue, (char*)&message, sizeof(messaging::LogMessage), 0);
        int fd = recv_fd(receiving_socket);
        if (fd == -1)
        {
            continue;
        }
        
        Client client = Client(fd, _message_queue, _ssl_context);

        while (client.is_open())
        { 
            message = messaging::createLogMessage(LogLevel::Info, "PID: " + std::to_string(getpid()) + ", IP: " + client.get_ip() + ":" + client.get_port() + " connected");
            mq_send(_message_queue, (char*)&message, sizeof(messaging::LogMessage), 0);
            
            Request request = Request();
            if (!client.receive_headers(request))
            {
                client.close();
                continue;
            }

            message = messaging::createLogMessage(LogLevel::Info, client.get_ip() + ":" + client.get_port() + " -> [" + request.get_type_as_string() + "]: " + request.get_path());
            mq_send(_message_queue, (char*)&message, sizeof(messaging::LogMessage), 0);

            if (request.get_type() == RequestType::POST)
            {
                client.handle_post(request);
            }
            else
            {
                Response response = Response();
                if (request.has_path() && request.is_script())
                {
                    client.execute_script(request);
                }
                else if (request.has_path())
                {
                    response.create_file_response(request.get_path());
                    client.send(response);
                }
                else
                {
                    response.create_file_response("/index.html");
                    client.send(response);
                }
            }

            if (!request.keep_alive())
            {
                client.close();
            }
        }
        pid_t pid = getpid();
        mq_send(_available_clients, (char*)&pid, sizeof(pid_t), 0);
    }
}

void Server::start_logger_process()
{
    _logger->log(LogLevel::Debug, std::time(nullptr), "Logger process started with PID: " + std::to_string(getpid()));
    while (!Server::_should_stop)
    {
        messaging::LogMessage message;
        ssize_t bytes_read = mq_receive(_message_queue, (char*)&message, sizeof(message), NULL);
        if (bytes_read == -1)
        {
            continue;
        }
        _logger->log(message._level, message._time, message._message);
    }

    _logger->log(LogLevel::Debug, std::time(nullptr), "Logger process ended");
}

void Server::start_client_processes()
{
    struct mq_attr attr_proc;
    attr_proc.mq_flags = O_NONBLOCK;
    attr_proc.mq_maxmsg = _max_connections;
    attr_proc.mq_msgsize = sizeof(pid_t);
    attr_proc.mq_curmsgs = 0; 

    _available_clients = mq_open("/available", O_CREAT | O_RDWR, 0666, &attr_proc);
    if (_available_clients == -1)
    {
        perror("mq_open");
        exit(EXIT_FAILURE);
    }

    _client_processes = std::vector<pid_t>(_max_connections);
    _socket_pairs = std::vector<std::array<int, 2>>(_max_connections);
    
    for (size_t i = 0; i < _max_connections; i++)
    {
        if (socketpair(AF_UNIX, SOCK_STREAM, 0, _socket_pairs[i].data()) == -1)
        {
            perror("socketpair");
            exit(EXIT_FAILURE);
        }

        pid_t pid = fork();
        if (pid != 0)
        {
            if (pid < 0)
            {
                messaging::LogMessage message = messaging::createLogMessage(LogLevel::Error, "fork: " + std::string(strerror(errno)));
                mq_send(_message_queue, (char*)&message, sizeof(messaging::LogMessage), 0);
                continue;
            }
            close(_socket_pairs[i][1]);
            mq_send(_available_clients, (char*)&pid, sizeof(pid_t), 0);
            _client_processes[i] = pid;
        }
        else
        {
            struct sigaction sa_2;
            sa_2.sa_handler = Server::cancellation_requested;
            sigemptyset(&sa_2.sa_mask);
            sa_2.sa_flags = 0;
            if (sigaction(SIGQUIT, &sa_2, NULL) == -1)
            {
                perror("sigaction");
                exit(EXIT_FAILURE);
            }
            signal(SIGINT, SIG_IGN);
            close(STDIN_FILENO);
            close(_socket_pairs[i][0]);
            handle_communication(_socket_pairs[i][1]);
            close(_socket_pairs[i][1]);
            exit(EXIT_SUCCESS);
        }
    }
    
}

// void Server::set_up_socket()
// {
//     if ((_server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
//         perror("socket failed");
//         exit(EXIT_FAILURE);
//     }

//     // Forcefully attaching socket to the port 8080
//     if (setsockopt(_server_fd, SOL_SOCKET,
//                    SO_REUSEADDR | SO_REUSEPORT, &_opt,
//                    sizeof(_opt))) 
//     {
//         perror("setsockopt");
//         exit(EXIT_FAILURE);
//     }
//     _address.sin_family = AF_INET;
//     _address.sin_addr.s_addr = INADDR_ANY;
//     _address.sin_port = htons(_port);

//     // Forcefully attaching socket to the port 8080
//     if (bind(_server_fd, (struct sockaddr*)&_address, sizeof(_address)) < 0)
//     {
//         perror("bind failed");
//         exit(EXIT_FAILURE);
//     }

//     if (listen(_server_fd, _max_connections) < 0)
//     {
//         perror("listen");
//         exit(EXIT_FAILURE);
//     }
// }

void Server::set_up_socket()
{
    if ((_server_fd = socket(AF_INET6, SOCK_STREAM, 0)) < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Allow reusing the address and port
    if (setsockopt(_server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &_opt, sizeof(_opt))) 
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    // Disable IPV6_V6ONLY so the socket accepts both IPv4 and IPv6 connections
    int option = 0;
    if (setsockopt(_server_fd, IPPROTO_IPV6, IPV6_V6ONLY, &option, sizeof(option)) < 0) 
    {
        perror("setsockopt IPV6_V6ONLY failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in6 address;
    memset(&address, 0, sizeof(address));
    address.sin6_family = AF_INET6;
    address.sin6_addr = in6addr_any;  // Accept connections on all available interfaces
    address.sin6_port = htons(_port);

    // Bind the socket
    if (bind(_server_fd, (struct sockaddr*)&address, sizeof(address)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Start listening for incoming connections
    if (listen(_server_fd, _max_connections) < 0)
    {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }
}

Server::Server()
{
    SSL_load_error_strings();
    SSL_library_init();
    OpenSSL_add_ssl_algorithms();

    _logger = new CompositeLogger();

    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = sizeof(messaging::LogMessage);
    attr.mq_curmsgs = 0; 
    
    _message_queue = mq_open("/logger", O_CREAT | O_RDWR, 0666, &attr);
    if (_message_queue == -1)
    {
        perror("mq_open");
        exit(EXIT_FAILURE);
    }
}

Server::~Server()
{
    SSL_CTX_free(_ssl_context);
    ERR_free_strings();
    EVP_cleanup();
    CRYPTO_cleanup_all_ex_data();

    mq_close(_available_clients);
    mq_unlink("/available");

    mq_close(_message_queue);
    mq_unlink("/logger");
    
    close(_server_fd);

    delete _logger;
}

Server *Server::get_instance()
{
    if (_instance == nullptr)
        _instance = new Server();
    return _instance;
}

Server *Server::configure_max_connections(int max_connections)
{
    _max_connections = max_connections;
    return this;
}

Server *Server::configure_port(uint16_t port)
{
    if (port < 1024 || port > 65535)
    {
        std::cout << "Invalid port number. Please choose a port between 1024 and 65535" << std::endl;
        exit(EXIT_FAILURE);
    }

    _port = port;
    return this;
}

Server *Server::configure_ssl(std::string certificate_path, std::string private_key_path)
{
    _ssl_context = SSL_CTX_new(TLS_server_method());
    if (!_ssl_context)
    {
        perror("SSL_CTX_new");
        exit(EXIT_FAILURE);
    }
    
    if (SSL_CTX_use_certificate_file(_ssl_context, certificate_path.c_str(), SSL_FILETYPE_PEM) <= 0) 
    {
        perror("SSL_CTX_use_certificate_file");
        exit(EXIT_FAILURE);
    }

    if (SSL_CTX_use_PrivateKey_file(_ssl_context, private_key_path.c_str(), SSL_FILETYPE_PEM) <= 0 ) 
    {
        perror("SSL_CTX_use_PrivateKey_file");
        exit(EXIT_FAILURE);
    }

    return this;
}

Server *Server::add_console_logger()
{
    _logger->add_logger(new Logger());
    return this;
}

Server *Server::add_logger(std::string file_path)
{
    _logger->add_logger(new Logger(file_path));
    return this;
}

void Server::run()
{
    _logger_pid = fork();
    if (_logger_pid != 0)
    {
        if (_logger_pid < 0)
        {
            perror("fork");
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        close(STDIN_FILENO);
        struct sigaction sa_2;
        sa_2.sa_handler = Server::cancellation_requested;
        sigemptyset(&sa_2.sa_mask);
        sa_2.sa_flags = 0;
        if (sigaction(SIGQUIT, &sa_2, NULL) == -1)
        {
            perror("sigaction");
            exit(EXIT_FAILURE);
        }
        signal(SIGINT, SIG_IGN);
        start_logger_process();
        exit(EXIT_SUCCESS);
    }
    set_up_socket();
    start_client_processes();

    struct sigaction sa_2;
    sa_2.sa_handler = Server::cancellation_requested;
    sigemptyset(&sa_2.sa_mask);
    sa_2.sa_flags = 0;
    if (sigaction(SIGINT, &sa_2, NULL) == -1)
    {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    messaging::LogMessage pid_message = messaging::createLogMessage(LogLevel::Info, "Server PID: " + std::to_string(getpid()));
    mq_send(_message_queue, (char*)&pid_message, sizeof(messaging::LogMessage), 0);

    messaging::LogMessage message = messaging::createLogMessage(LogLevel::Info, "Server started on port " + std::to_string(_port));
    mq_send(_message_queue, (char*)&message, sizeof(messaging::LogMessage), 0);
    
    while(!Server::_should_stop)
    {
        message = messaging::createLogMessage(LogLevel::Debug, "Waiting for connection");
        mq_send(_message_queue, (char*)&message, sizeof(messaging::LogMessage), 0);

        int new_socket = accept(_server_fd, (struct sockaddr*)&_address, &addrlen);
        if (new_socket == -1)
        {
            message = messaging::createLogMessage(LogLevel::Error, "accept: " + std::string(strerror(errno)));
            mq_send(_message_queue, (char*)&message, sizeof(messaging::LogMessage), 0);
            continue;
        }

        pid_t pid;
        if (mq_receive(_available_clients, (char*)&pid, sizeof(pid_t), NULL) == -1)
        {
            message = messaging::createLogMessage(LogLevel::Error, "mq_receive: " + std::string(strerror(errno)));
            mq_send(_message_queue, (char*)&message, sizeof(messaging::LogMessage), 0);

            Client client = Client(new_socket, _message_queue, _ssl_context);
            message = messaging::createLogMessage(LogLevel::Warning, "Service unavailable");
            mq_send(_message_queue, (char*)&message, sizeof(messaging::LogMessage), 0);

            Response response = Response();
            response.create_service_unavailable_response();
            client.send(response);

            message = messaging::createLogMessage(LogLevel::Debug, "Service unavailable response sent");
            mq_send(_message_queue, (char*)&message, sizeof(messaging::LogMessage), 0);
            client.close();
            continue;
        }

        int client_index = std::distance(_client_processes.begin(), std::find(_client_processes.begin(), _client_processes.end(), pid));
        if (send_fd(_socket_pairs[client_index][0], new_socket) == -1)
        {
            message = messaging::createLogMessage(LogLevel::Error, "send_fd: " + std::string(strerror(errno)));
            mq_send(_message_queue, (char*)&message, sizeof(messaging::LogMessage), 0);
            mq_send(_available_clients, (char*)&pid, sizeof(pid_t), 0);
            close(new_socket);
        }
        message = messaging::createLogMessage(LogLevel::Debug, "Connection sent to PID: " + std::to_string(_client_processes[client_index]));
        mq_send(_message_queue, (char*)&message, sizeof(messaging::LogMessage), 0);
    }

    message = messaging::createLogMessage(LogLevel::Info, "Server shutting down before killing child processes");
    mq_send(_message_queue, (char*)&message, sizeof(messaging::LogMessage), 0);
    for (size_t i = 0; i < _max_connections; i++)
    {
        close(_socket_pairs[i][0]);
        kill(_client_processes[i], SIGQUIT);
        pid_t pid = waitpid(_client_processes[i], NULL, 0);
        message = messaging::createLogMessage(LogLevel::Debug, "Child process with PID: " + std::to_string(pid) + " killed");
        mq_send(_message_queue, (char*)&message, sizeof(messaging::LogMessage), 0);
    }

    sleep(1);

    kill(_logger_pid, SIGQUIT);
    waitpid(_logger_pid, NULL, 0);
}
