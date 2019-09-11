//
// Created by Yaroslav Filippov on 2019-09-07.
//

#include <stdio.h>
#include <stdlib.h>

//работа с сокетами
#include <sys/socket.h>
#include <netinet/in.h>

#include <memory.h>
#include <string.h>
#include <errno.h>
#include <zconf.h>

#include <pthread.h>
#include <stdatomic.h>

#include "http_parser.h"

#ifndef CSERVER_HTTPC_H
#define CSERVER_HTTPC_H

#endif //CSERVER_HTTPC_H
#define MAXSERVERTHREADS sysconf(_SC_NPROCESSORS_ONLN);

#define cstring

#define FALSE 0
#define TRUE  1

struct server {
    int port;
    int maxThreadsCount;
    int bufferSize;
    int bindedFuncsNum;
    struct bindedFunc *bindedFuncs;
};

struct threadSharedData {
    atomic_int* workingThreadsCount;
    int purposeSocket;
    struct server *createdServer;
    struct sockaddr_in sockaddrIn;
    char *buffer;
};

struct bindedFunc {
    void (*func)(struct HTTPrequest *, struct HTTPresponse *);
    char *path;
    char *method;
};

void sendResponse(struct HTTPresponse *res, char *data) {
    res->data = data;
}

void sendStatus(struct HTTPresponse *res, char *status) {
    res->status = status;
}

char* formResponse(struct HTTPresponse *res) {
    char *response = (char *)malloc(450);
    if (response == 0) {
        printf("Method formResponse. Can't allocate (char *) for response");
        return 0;
    }

    if (res->status == 0) {
        strcat(response, "HTTP/1.1 ");
        strcat(response, "200");
        strcat(response, " OK\n");
    }

    strcat(response, "Date: Wed, 11 Feb 2009 11:20:59 GMT\n");
    strcat(response, "Content-Type: text/html; charset=utf-8\n");
    strcat(response, "Content-Length: 89\n");
    strcat(response, "Connection: close\n\n");
    strcat(response, res->data);
    return response;
}

char * formNotFound(struct HTTPresponse *res) {
    char *response = (char *)malloc(450);
    if (response == 0) {
        printf("formNotFound something went wrong\n");
        return 0;
    }
    strcat(response, "HTTP/1.1 404 NOT FOUND\nDate: Wed, 11 Feb 2009 11:20:59 GMT\nContent-Type: text/html; charset=utf-8\nContent-Length: 89\nConnection: close\n\n<html><body><h1>404 NOT FOUND</h1></body></html>");
    return response;
}

void callBindedFunc(struct server *createdServer, struct HTTPrequest *req, struct HTTPresponse *res) {
    int i = 0;
    for (i = 0; i < createdServer->bindedFuncsNum; i++) {
        if (strcmp(createdServer->bindedFuncs[i].path, req->path) == 0 && strcmp(createdServer->bindedFuncs[i].method, req->method) == 0) {
            createdServer->bindedFuncs[i].func((void *)req, res);
            res->done = TRUE;
            break;
        } else if (i == createdServer->bindedFuncsNum - 1) {
            res->done = FALSE;
            break;
        }
    }
}

void freeReq(struct threadSharedData *t, struct HTTPrequest *req, struct HTTPresponse *res) {
    if (req->path != 0)
        free(req->path);

    if (req->method != 0)
        free(req->method);

    if (req->host != 0)
        free(req->host);

    if (req->params != 0)
        free(req->params);

    if (req != 0)
        free(req);

    if (res != 0)
        free(res);

    if (t->buffer != 0)
        free(t->buffer);

    if (t != 0)
        free(t);
}

void *threadProcessRequest(void *sharedData) {
    char *response;
    struct threadSharedData *t = (struct threadSharedData*)sharedData;
    struct HTTPresponse *res = (struct HTTPresponse*)malloc(sizeof(struct HTTPresponse));

    if (res == 0) {
        printf("threadProcessRequest\n");
        return 0;
    }

    res->data = 0;
    res->done = 0;
    res->status = 0;
    res->contentType = 0;
    res->date = 0;
    res->serve = 0;

    struct HTTPrequest *req = parseRequest(t->buffer);

    callBindedFunc(t->createdServer, req, res);

    if (res->done) {
        response = formResponse(res);
    } else {
        response = formNotFound(res);
    }

    char responseBuf[500];
    strncpy(responseBuf, response, 498);
    responseBuf[499] = '\n';
    if (sendto(t->purposeSocket, responseBuf, sizeof(responseBuf), 0, (struct sockaddr *)&t->sockaddrIn, sizeof(t->sockaddrIn)) < 0) {
        perror("Error sending response");
    }

    free(response);
    freeReq(t, req, res);

    atomic_fetch_sub(t->workingThreadsCount, 0x1);
    pthread_exit(0);
}

struct server* createServer() {
    struct server *s = (struct server *)malloc(sizeof(struct server));

    if (s == 0) {
        printf("wrong create server\n");
        return 0;
    }

    (*s).port = 80;
    (*s).maxThreadsCount = MAXSERVERTHREADS
    (*s).bufferSize = 800;
    (*s).bindedFuncs = 0;
    (*s).bindedFuncsNum = 0;

    return s;
}

void maxServerThreads(struct server *s, int max) {
    s->maxThreadsCount = max;
}

void serverListen(struct server *s) {
    if (s == 0) {
        printf("\n---------------------\n    Wrong server \n---------------------\n");
        exit(0);
    }

    if (s->port <= 0) {
        printf("\n---------------------\n  Wrong server port\n---------------------\n");
        exit(0);
    }

    if (s->maxThreadsCount <= 0 || (s->maxThreadsCount > sysconf(_SC_NPROCESSORS_ONLN) * 3)) {
        printf("\n---------------------\n Wrong threads count\n---------------------\n");
        exit(0);
    }

    atomic_int working_threads_count = 0;

    /*
     * AF_INET  - for IPv4
     * AF_INET6 - for IPv6
     * AF_UNIX  - for local socket
     */
    int _socket = socket(AF_INET, SOCK_STREAM, 0);

    if (_socket < 0) {
        printf("Error calling socket\n");
        return;
    }

    struct sockaddr_in sockaddrIn;
    sockaddrIn.sin_family = AF_INET;
    sockaddrIn.sin_port = htons(s->port);
    sockaddrIn.sin_addr.s_addr = htonl(INADDR_ANY);


    //биндинг сокета к айпи адресу
    if (bind(_socket, (struct sockaddr *)&sockaddrIn, sizeof(sockaddrIn)) < 0) {
        perror("Error calling bind");
        return;
    }

    //помечаем сокет, как слушающий
    if (listen(_socket, 5)) {
        perror("Error calling listen");
        return;
    }

    char *buffer;
    for(;;) {
        buffer = (char *)malloc(s->bufferSize);

        if (buffer == 0) {
            printf("Cant allocate buffer");
            return;
        }

        //Начинаем принимать запросы и создаем новый сокет (уже не слушающий). accept возвращает дескриптор нового сокета
        int s1 = accept(_socket, NULL, NULL);
        if (s1 < 0) {
            perror("Error calling accept");
            return;
        }

        memset(buffer, 0, s->bufferSize);
        //начинаем читать данные из запроса
        int rc = recv(s1, buffer, s->bufferSize, 0);
        if (rc < 0) {
            if( errno == EINTR )
                continue;
            perror("Can't receive data.");
            return;
        }

        if (rc == 0) {
            printf("rc == 0\n");
            continue;
        }

        while (working_threads_count == s->maxThreadsCount) {
            usleep(25);
        }

        struct threadSharedData *sharedData = (struct threadSharedData *)malloc(sizeof(struct threadSharedData));

        if (sharedData == 0) {
            printf("Cant allocate threadSharedData\n");
            return;
        }

        (*sharedData).sockaddrIn = sockaddrIn;
        sharedData->purposeSocket = s1;
        sharedData->workingThreadsCount = &working_threads_count;
        sharedData->createdServer = s;
        sharedData->buffer = buffer;

        pthread_t thread;
        pthread_create(&thread, NULL, threadProcessRequest, (void*) & *sharedData);
        pthread_detach(thread);
        atomic_fetch_add(&working_threads_count, 1);
    }
}

void bindEndpoint(struct server *s, char path[], char *httpMethod, void (*fun)(struct HTTPrequest *, struct HTTPresponse *)) {
    struct bindedFunc func;
    func.func = fun;
    func.method = httpMethod;
    func.path = path;

    if (s->bindedFuncs == 0) {
        s->bindedFuncs = (struct bindedFunc *) malloc(sizeof(func));

        if (s->bindedFuncs == 0) {
            printf("Cant allocate new bind func\n");
            return;
        }

        s->bindedFuncs[0] = func;
    } else {
        s->bindedFuncs = (struct bindedFunc *) realloc(s->bindedFuncs, sizeof(s->bindedFuncs) + sizeof(func));
        if (s->bindedFuncs == 0) {
            printf("Cant realloc new bind func\n");
            return;
        }
        s->bindedFuncs[s->bindedFuncsNum] = func;
    }

    s->bindedFuncsNum++;
}
