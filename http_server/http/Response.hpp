#include <string>
#include <fstream>
#include <iostream>
#include <map>

class Response
{
private:
    const std::string _version = "HTTP/1.1";
    const std::string _default_directory = "www";
    const std::map<std::string, std::string> _mime_types = {
        {".html", "text/html"},
        {".css", "text/css"},
        {".js", "text/javascript"},
        {".jpg", "image/jpeg"},
        {".jpeg", "image/jpeg"},
        {".png", "image/png"},
        {".gif", "image/gif"},
        {".bmp", "image/bmp"},
        {".ico", "image/x-icon"},
        {".svg", "image/svg+xml"},
        {".pdf", "application/pdf"},
        {".json", "application/json"},
        {".xml", "application/xml"},
        {".zip", "application/zip"},
        {".gzip", "application/gzip"},
        {".mp3", "audio/mpeg"},
        {".wav", "audio/wav"},
        {".mp4", "video/mp4"},
        {".mpeg", "video/mpeg"},
        {".webm", "video/webm"},
        {".ogg", "video/ogg"},
        {".flv", "video/x-flv"}
    };
    
    std::string _response;
    std::string _content;
    std::string _mime_type = "text/html";
    int _content_length;

    bool load_file(std::string path);
    void set_mime_type(std::string path);
public:
    Response();
    ~Response();
    std::string get_response();
    void create_file_response(std::string path);
    void create_not_found_response();
    int get_response_length();
};
