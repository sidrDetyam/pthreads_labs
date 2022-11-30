#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <poll.h>
#include <limits.h>

#include "../common.h"

enum STATUS{
    ERROR = -1,
    SUCCESS = 0
};

#define ASSERT_RETURN(cond__) \
do{\
    if(!(cond__)){ \
        return ERROR; \
    } \
}while(0)

#define ASSERT_RETURN_C(cond__, on_fail__) \
do{\
    if(!(cond__)){ \
        on_fail__;    \
        return ERROR; \
    } \
}while(0)


struct SERVER_SOCKET{
    int fd;
    struct sockaddr_in addr;
    socklen_t socklen;
};
typedef struct SERVER_SOCKET servsock_t;


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

int close_servsock(servsock_t* sock){
    ASSERT_RETURN(shutdown(sock->fd, SHUT_RDWR) == 0);
    return SUCCESS;
}

int accept_servsock(servsock_t* servsock){
    int fd = accept(servsock->fd, (struct sockaddr*)& servsock->addr, &servsock->socklen);
    return (fd >= 0? fd : ERROR);
}

static int
name2addr(const char* host_name, uint16_t host_port, struct sockaddr_in* sock_addr){
    struct hostent *server = gethostbyname(host_name);
    ASSERT_RETURN(server != NULL);

    memset(&sock_addr->sin_zero, 0, sizeof(sock_addr->sin_zero));
    sock_addr->sin_family = AF_INET;
    sock_addr->sin_port = htons(host_port);
    memcpy(&sock_addr->sin_addr.s_addr,server->h_addr_list[0], server->h_length);

    return SUCCESS;
}


int main(){

    char buffer[1024] = { 0 };
    char* hello = "Hello from server";

    servsock_t servsock;
    ASSERT(create_servsock(8080, 3, &servsock) == SUCCESS);
    int fd;
    ASSERT((fd = accept_servsock(&servsock)) != ERROR);

    read(fd, buffer, 1024);
    printf("%s\n", buffer);
    send(fd, hello, strlen(hello), 0);
    printf("Hello message sent\n");

    close(fd);
    close_servsock(&servsock);

    return 0;
}