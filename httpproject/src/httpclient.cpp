#include "/home/u/httpclient/src/httpclient.h"
#include <iostream>

// HttpClient::HttpClient()
// {
//     std::cout << "this is the consturctor" << std::endl;
// }

bool HttpClient::Request(std::string host, std::string port, std::string method, std::string body, std::string response)
{
    if (host.empty() || port.empty() || method.empty() || body.empty() || response.empty())
    {
        return false;
    }
    
    return false;
}