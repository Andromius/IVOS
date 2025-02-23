#include <map>
#include <string>
#include <vector>
#include "RequestType.hpp"
#include <sstream>
#include <algorithm>

class Request
{
private:
    const std::map<std::string, RequestType> _string_to_type = { 
        {"GET", GET},
        {"POST", POST},
        {"OPTIONS", OPTIONS},
        {"HEAD", HEAD},
        {"PUT", PUT},
        {"DELETE", DELETE}
    };
    
    const std::map<RequestType, std::string> _type_to_string = {
        {GET, "GET"},
        {POST, "POST"},
        {OPTIONS, "OPTIONS"},
        {HEAD, "HEAD"},
        {PUT, "PUT"},
        {DELETE, "DELETE"}
    };

    RequestType _type;
    std::string _path;
    std::string _version;
    std::map<std::string, std::string> _fields;

    std::vector<std::string> string_split(std::string& input, std::string delimiter);
    RequestType to_request_type(std::string& input);
public:
    Request();
    ~Request();

    void parse(std::string request);
    RequestType get_type();
    std::string get_type_as_string();
    bool has_path();
    std::string get_path();
};
