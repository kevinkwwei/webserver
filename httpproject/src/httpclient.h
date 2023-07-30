#pragma once
#include <vector>
#include <string>
#include <memory>
#include <iostream>
#include <stdio.h>
#include <stdint.h>

struct client_impl {
    bool is_connected;
    std::string send_buffer;
    int64_t socket;

    bool Connect(std::string host, std::string port);
};

class HttpClient : std::enable_shared_from_this<HttpClient>
{
public:
    explicit HttpClient() { std::cout << "this is the consturctor" << std::endl; }
    ~HttpClient() = default;
    bool Request(std::string host, std::string port, std::string method, std::string body, std::string response);
    bool Post(std::string host, std::string port, std::string method, std::string body, std::string response);
    bool Get(std::string host, std::string port, std::string method, std::string body, std::string response);

private:
};