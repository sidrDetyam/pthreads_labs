#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <poll.h>
#include <limits.h>


#include "common.h"
#include "server_socket.h"
#include "http_header_parser.h"


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

    char *buffer = malloc(100000);
    memset(buffer, 0, 100000);
    char* hello = "Hello from server";

    servsock_t servsock;
    ASSERT(create_servsock(8080, 3, &servsock) == SUCCESS);
    int fd;
    ASSERT((fd = accept_servsock(&servsock)) != ERROR);

    read(fd, buffer, 100000);
    printf("%s\n", buffer);

    parser_t parser;
    parser_init(&parser, buffer);
    while(1){
        int status = parse_next(&parser);
        if(status == PARSED){
            break;
        }
        if(status != HAS_NEXT){
            printf("bruuh\n");
            break;
        }
    }

    for(size_t i=0; i < parser.headers.cnt; ++i){
        header_t* header = vheader_t_get(&parser.headers, i);
        write(1, header->type, header->tlen);
        printf("  ::::  ");
        fflush(stdout);
        write(1, header->value, header->vlen);
        printf("\n");
    }

    close(fd);
    close_servsock(&servsock);

    return 0;
}
