//
// Created by Yaroslav Filippov on 2019-09-07.
//

#ifndef CSERVER_HTTP_PARSER_H
#define CSERVER_HTTP_PARSER_H

#endif //CSERVER_HTTP_PARSER_H

struct HTTPrequest {
    char *method;
    char *path;
    char *params;
    char *host;
};

struct HTTPresponse {
    char *status;
    char *date;
    char *serve;
    char *contentType;
    char *data;
    _Bool done;
};

void parsePath(struct HTTPrequest *req, char *t) {
    char *token, *str, *tofree;

    tofree = str = strdup(t);
    while ((token = strsep(&str, "?"))) {
        if (req->path == 0) {
            req->path = strdup(token);
        } else {
            req->params = strdup(token);
        }
    }

    if (req->path == 0) {
        req->path = strdup(token);
    }

    free(tofree);
}

void parseRequestLine(struct HTTPrequest *req, char *t) {
    char *token, *str, *tofree, *previousToken;

    char *hostStr = "Host:";
    tofree = str = strdup(t);
    if (strlen(str) == 0) {
        return;
    }

    while ((token = strsep(&str, " "))) {
        if (req->method == 0) {
            req->method = strdup(token);
            previousToken = token;
            token = strsep(&str, " ");

            if (token == 0)
                continue;

            parsePath(req, token);
            continue;
        }

        if (req->params == 0 && previousToken != 0 && strcmp(previousToken, hostStr) == 0) {
            req->host = strdup(token);
            previousToken = token;
            continue;
        }

        previousToken = token;
    }
    free(tofree);
}

struct HTTPrequest* parseRequest(char *buffer) {
    struct HTTPrequest *req = (struct HTTPrequest*)malloc(sizeof(struct HTTPrequest));
    req->params = 0;
    req->host = 0;
    req->method = 0;
    req->path = 0;

    char *token, *str, *tofree;

    tofree = str = strdup(buffer);

    if (strlen(str) == 0) {
        exit(0);
    }
    while ((token = strsep(&str, "\n"))) {
        parseRequestLine(req, token);
    }
    free(tofree);
    return req;
}