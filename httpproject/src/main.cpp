
#include "httpserver/http_server.h"

#define MAX_LINE 60
#define SERVER_PORT 6666
using namespace std;
int main()
{
    auto http_server = std::make_shared<HttpServer>();
    http_server->Start();
    return 0;
}

