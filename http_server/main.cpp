// Server side C program to demonstrate Socket
// programming
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>

#include <fstream>
#include <iostream>

#include "http/Client.hpp"

#define PORT 8080

int main(int argc, char const* argv[])
{
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    socklen_t addrlen = sizeof(address);

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Forcefully attaching socket to the port 8080
    if (setsockopt(server_fd, SOL_SOCKET,
                   SO_REUSEADDR | SO_REUSEPORT, &opt,
                   sizeof(opt))) 
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Forcefully attaching socket to the port 8080
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    while((new_socket = accept(server_fd, (struct sockaddr*)&address, &addrlen)) != -1)
    {
        Client client = Client(new_socket);
        while (client.is_open())
        {
            Request request = Request();
            if (!client.receive(request))
                continue;

            Response response = Response();
            if (request.has_path())
            {
                response.create_file_response(request.get_path());
                client.send(response);
                continue;
            }
    
            response.create_file_response("/index.html");
            client.send(response);
        }
    }

    close(server_fd);
    
    return 0;
}
