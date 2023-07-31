//
// Created by Kevin Wei on 2023/8/1.
//

#pragma once
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <memory>

class HttpServer : std::enable_shared_from_this<HttpServer> {
public:
    HttpServer();
    ~HttpServer() = default;
    void Start();

};

