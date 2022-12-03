//
// Created by argem on 03.12.2022.
//

#ifndef PTHREAD___CONNECTION_HANDLER_H
#define PTHREAD___CONNECTION_HANDLER_H

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "http_header_parser.h"


#define ELEMENT_TYPE char
#include "cvector_def.h"


enum HandlingStep{
    PARSING_REQ_TYPE = 1337,
    PARSING_REQ_HEADERS,
    PARSING_REQ_BODY,

    PARSING_RESP_CODE,
    PARSING_RESP_HEADERS,
    PARSING_RESP_BODY,

    HANDLED,
    HANDLER_EXCEPTIONALLY
};


struct HttpConnectionHandlerContext{
    int client_fd;
    int server_fd;
    int client_events;
    int server_events;

    request_t request;
    response_t response;

    vchar cbuff;
    char* cppos;
    //char* crpos;

    vchar sbuff;
    char* sppos;
    //char* srpos;

    int handling_step;
};
typedef struct HttpConnectionHandlerContext handler_context_t;


void
init_context(handler_context_t* context, int client_fd);

void
destroy_context(handler_context_t* context);

void
handle(handler_context_t* context, int fd, int events);

int
name2addr(const char *host_name, uint16_t host_port, struct sockaddr_in *sock_addr);


#endif //PTHREAD___CONNECTION_HANDLER_H
