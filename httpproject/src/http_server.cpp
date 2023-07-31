//
// Created by Kevin Wei on 2023/8/1.
//

#include "http_server.h"

#define MAX_LINE 60
#define SERVER_PORT 6666
HttpServer::HttpServer() {
    std::cout << "this is the constructor " << std::endl;
}

void HttpServer::Start() {
    std::cout << "hello world" << std::endl;
    std::cout << "this is the first kevin linux step" << std::endl;
    // std::shared_ptr<HttpClient> http_client = std::make_shared<HttpClient>();
    std::cout << "a really simple server code to use" << std::endl;

    struct sockaddr_in server_addr, client_addr;
    socklen_t cliaddr_len;
    int listend_fd, conn_fd;
    char buf[MAX_LINE];
    char str[INET_ADDRSTRLEN];
    int i, n;
    listend_fd = socket(AF_INET, SOCK_STREAM, 0);
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    bind(listend_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    listen(listend_fd, 20);

    std::cout << "accepting .........." << std::endl;

    while (1)
    {
        cliaddr_len = sizeof(client_addr);
        conn_fd = accept(listend_fd, (struct sockaddr *)&client_addr, &cliaddr_len);
        n = read(conn_fd, buf, MAX_LINE);
        std::cout << "recv from port" << SERVER_PORT << std::endl;
        inet_ntop(AF_INET, &client_addr.sin_addr, str, sizeof(str)),
                ntohs(client_addr.sin_port);
        for (int i = 0; i < n; i++) {
            buf[i] = toupper(buf[i]);
        }
        write(conn_fd, buf, n);
        close(conn_fd);
    }

}