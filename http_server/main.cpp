#include "core/Server.hpp"

int main(int argc, char const* argv[])
{
    strncasecmp("hello", "Hello", 5);
    
    for (int i = 1; i < argc; i++)
    {
        if (strcasecmp(argv[i],"-h") == 0 || strcasecmp(argv[i],"--help") == 0)
        {
            std::cout << "Usage: " << argv[0] << " [options]" << std::endl;
            std::cout << "Options:" << std::endl;
            std::cout << "  -h, --help\t\tShow this help message and exit" << std::endl;
            std::cout << "  -p, --port\t\tSet the port number" << std::endl;
            std::cout << "  -c, --connections\tSet the maximum number of connections" << std::endl;
            std::cout << "  -l, --log\t\tSet the log file" << std::endl;
            std::cout << "  -s, --ssl\t\tSet the SSL certificate and private key" << std::endl;
            exit(EXIT_SUCCESS);
        }
        else if (strcasecmp(argv[i],"-p") == 0 || strcasecmp(argv[i],"--port") == 0)
        {

        }
        else if (strcasecmp(argv[i],"-c") == 0 || strcasecmp(argv[i],"--connections") == 0)
        {

        }
        else if (strcasecmp(argv[i],"-l") == 0 || strcasecmp(argv[i],"--log") == 0)
        {

        }
        else if (strcasecmp(argv[i],"-s") == 0 || strcasecmp(argv[i],"--ssl") == 0)
        {

        }
        else
        {
            std::cerr << "Unknown option: " << argv[i] << std::endl;
            exit(EXIT_FAILURE);
        }
    }

    Server* server = Server::get_instance()
        ->configure_port(8080)
        ->configure_max_connections(10)
        ->add_logger("server.log")
        ->add_console_logger()
        ->configure_ssl("certificate.pem", "private.pem");
    server->run();
    delete server;
    exit(EXIT_SUCCESS); 
}
