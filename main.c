#include "httpc.h"

void actor(struct HTTPrequest *req, struct HTTPresponse *res) {
    sendResponse(res, "<html><body><a href='http://example.com/sabout.html#contacts'>Actor</a></body></html>");
    printf("path: %s\n:", req->path);
    printf("method: %s\n:", req->method);
    printf("host: %s\n:", req->host);
    printf("params: %s\n:", req->params);
}

void task(struct HTTPrequest *req, struct HTTPresponse *res) {
    sendResponse(res, "<html><body><a href='http://example.com/sabout.html#contacts'>Task!</a></body></html>");
    printf("path: %s\n:", req->path);
    printf("method: %s\n:", req->method);
    printf("host: %s\n:", req->host);
    printf("params: %s\n:", req->params);
}

int main() {
    struct server *s;
    s = createServer();
    s->port = 80;

    bindEndpoint(s, "/actor", "GET", actor);
    bindEndpoint(s, "/task", "GET", task);

    serverListen(s);
    return 0;
}