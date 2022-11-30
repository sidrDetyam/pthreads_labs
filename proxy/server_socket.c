//
// Created by argem on 30.11.2022.
//
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "server_socket.h"
#include "common.h"


int
create_servsock(uint16_t port, int limit, servsock_t* sock){
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    ASSERT_RETURN(server_fd >= 0);

    int opt = 1;
    ASSERT_RETURN_C(setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) == 0,
                    close(server_fd));

    sock->addr.sin_family = AF_INET;
    sock->addr.sin_addr.s_addr = INADDR_ANY;
    sock->addr.sin_port = htons(port);
    sock->fd = server_fd;
    sock->socklen = (socklen_t) sizeof(sock->addr);

    ASSERT_RETURN_C(bind(server_fd, (struct sockaddr*) &sock->addr, sock->socklen) == 0,
                    close(server_fd));
    ASSERT_RETURN_C(listen(server_fd, limit) == 0,
                    close(server_fd));

    return SUCCESS;
}

int
close_servsock(servsock_t* sock){
    ASSERT_RETURN(shutdown(sock->fd, SHUT_RDWR) == 0);
    return SUCCESS;
}

int
accept_servsock(servsock_t* servsock){
    int fd = accept(servsock->fd, (struct sockaddr*)& servsock->addr, &servsock->socklen);
    return (fd >= 0? fd : ERROR);
}
