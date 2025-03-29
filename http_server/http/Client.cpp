#include "Client.hpp"

Client::Client(int socket, mqd_t message_queue, SSL_CTX* ssl_context)
{
    _socket = socket;
    _is_open = true;
    _message_queue = message_queue;
    _ssl = SSL_new(ssl_context);
    SSL_set_fd(_ssl, _socket);
    
    messaging::LogMessage message;
    if (SSL_accept(_ssl) <= 0)
    {
        message = messaging::createLogMessage(LogLevel::Error, "PID: " + std::to_string(getpid()) + " Failed to accept SSL connection code (" + std::to_string(SSL_get_error(_ssl, -1)) + ")");
        mq_send(_message_queue, (char*)&message, sizeof(messaging::LogMessage), 0);
        close();
        return;
    }
    getsockname(_socket, (struct sockaddr*)&_addr, &_addr_len);
    message = messaging::createLogMessage(LogLevel::Info, "Accepted SSL connection");
    mq_send(_message_queue, (char*)&message, sizeof(messaging::LogMessage), 0);
}

Client::~Client()
{
}

void Client::send(Response& response)
{
    if (!_is_open)
        return;
    SSL_write(_ssl, response.get_response().c_str(), response.get_response_length());
}

bool Client::receive_headers(Request& request)
{
    bzero(_buffer, 1024);
    _valread = SSL_read(_ssl, _buffer, 1024); // subtract 1 for the null terminator at the end
    if (_valread == -1)
    {
        close();
        return false;
    }
    else if (_valread == 0)
    {
        _peer_disconnected = true;
        close();
        return false;
    }
    
    bzero(_remainder, 1024);
    char *pos = strstr(_buffer, "\r\n\r\n");
    if (pos != NULL)
    {
        size_t offset = (pos + 4) - _buffer;
        _remaining_size = _valread - offset;
        memcpy(_remainder, _buffer + offset, _remaining_size);
        char *headers = new char[offset];
        memcpy(headers, _buffer, offset);
        request.parse(headers);
        delete headers;
    }


    return true;
}

void Client::handle_post(Request &request)
{
    char *pos = strstr(_remainder, "\r\n\r\n");
    size_t offset = (pos + 4) - _remainder;
    _remaining_size = _remaining_size - offset;

    bzero(_buffer, 1024);
    memcpy(_buffer, _remainder + offset, _remaining_size);

    int content_length = atoi(request.get_field("Content-Length").c_str());

    std::string sss = std::string(_remainder);
    std::string filename;
    std::vector<std::string> spl = Request::string_split(sss, "filename=");
    filename = Request::string_split(spl[1], "\r\n")[0];
    filename.erase(std::remove(filename.begin(), filename.end(), '\"'), filename.end());
    int curlength = _remaining_size + offset;
    int fd = open(filename.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
    write(fd, _buffer, _remaining_size);

    char filebuf[16384];
    
    while (curlength < content_length)
    {
        _valread = SSL_read(_ssl, filebuf, 16384);
        if (_valread == 0 || _valread == -1)
        {
            break;
        }

        pos = strstr(filebuf, request.get_boundary().c_str());
        if (pos)
        {
            char* start = pos + request.get_boundary().length() + 2;
            
            pos = strstr(start, "\r\n\r\n");
            size_t size = (pos + 4) - start;

            if (pos)
            {
                write(fd, filebuf, start - filebuf);
                ::close(fd);

                bzero(_buffer, 1024);
                memcpy(_buffer, start, size);

                sss = std::string(_buffer);
                spl = Request::string_split(sss, "filename=");
                filename = Request::string_split(spl[1], "\r\n")[0];
                filename.erase(std::remove(filename.begin(), filename.end(), '\"'), filename.end());
                
                fd = open(filename.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
                start = start + size;
                write(fd, start, _valread - (start - filebuf));
            }
            else
            {
                write(fd, filebuf, _valread - ((std::string("--") + request.get_boundary()).length() + 6));
            }
        }
        else
        {
            write(fd, filebuf, _valread);
        }

        curlength += _valread;
    }
    Response r = Response();
    r.create_created_response();
    send(r);
}

std::string Client::get_ip()
{
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &_addr.sin_addr, ip, INET_ADDRSTRLEN);
    return std::string(ip);
}

std::string Client::get_port()
{
    return std::to_string(ntohs(_addr.sin_port));
}

void Client::execute_script(Request& request)
{
    int pipe_fd[2]; 
    if (pipe(pipe_fd) == -1)
    {
        perror("pipe");
        return;
    }
    if (fork() == 0)
    {
        ::close(pipe_fd[0]);
        dup2(pipe_fd[1], STDOUT_FILENO);
        dup2(pipe_fd[1], STDERR_FILENO);
        ::close(pipe_fd[1]);
        execlp(_runners[request.get_path_extension()].c_str(), _runners[request.get_path_extension()].c_str(), ("www" + request.get_path()).c_str(), NULL);
        exit(EXIT_SUCCESS);
    }
    else
    {
        ::close(pipe_fd[1]);
        Response response = Response();
        std::string response_string = "";
        char buffer[1024];
        ssize_t bytes_read;
        while ((bytes_read = read(pipe_fd[0], buffer, 1024)) > 0)
        {
            response_string.append(buffer, bytes_read);
        }
        ::close(pipe_fd[0]);
        response.create_string_response(response_string);
        send(response);
        wait(NULL);
    }
}

void Client::close()
{
    if (!_is_open)
        return;
    if (!_peer_disconnected)
        SSL_shutdown(_ssl);
    SSL_free(_ssl);
    ::close(_socket);
    _is_open = false;
}

bool Client::is_open()
{
    return _is_open;
}


