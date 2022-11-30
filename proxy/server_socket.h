//
// Created by argem on 30.11.2022.
//

#ifndef PTHREAD_SERVER_SOCKET_H
#define PTHREAD_SERVER_SOCKET_H

#include <sys/socket.h>

struct SERVER_SOCKET{
    int fd;
    struct sockaddr_in addr;
    socklen_t socklen;
};
typedef struct SERVER_SOCKET servsock_t;

int
create_servsock(uint16_t port, int limit, servsock_t* sock);

int
close_servsock(servsock_t* sock);

int
accept_servsock(servsock_t* servsock);

#endif //PTHREAD_SERVER_SOCKET_H
