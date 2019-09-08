# httpc
#### Still WIP!
#### It's my simple lib implementation of C-server.

Usage
-----

Include `httpc.h` to your project. To create a server, use 
the method `createServer()` that will return the server instance.
Example:
```c
#include "httpc.h"

void actor(struct HTTPrequest *req, struct HTTPresponse *res) {
    sendResponse(res, "<html><body><a href='http://example.com/sabout.html#contacts'>Actor</a></body></html>");
}

void task(struct HTTPrequest *req, struct HTTPresponse *res) {
    sendResponse(res, "<html><body><a href='http://example.com/sabout.html#contacts'>Task!</a></body></html>");
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
```