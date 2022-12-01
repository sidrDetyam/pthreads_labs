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


struct ConnectionHandlerContext{
    int fd;
};
typedef struct ConnectionHandlerContext context_t;


void*
handler(void* arg){
    context_t* context = (context_t*) arg;

    char* buffer = malloc(1000000);
    char* rstart = buffer;
    char* rpos = buffer;
    const char* ppos = buffer;
    request_t req;
    request_init(&req);

    volatile int tparsed = 0;
    volatile int host_fd = -1;
    while(1){
        ssize_t cnt = read(context->fd, rpos, 100000);
        rpos += cnt;

        if(cnt == 0 || cnt == -1){
            printf("end: %zd\n", cnt);
            break;
        }

        if(!tparsed) {
            int status = parse_req_type(&ppos, &req);
            if (status == NO_END_OF_LINE) {
                continue;
            }
            if(status == PARSING_ERROR){
                printf("error\n");
                return NULL;
            }
            tparsed = 1;
        }

        header_t header;
        int status = parse_next_header(&ppos, &header);
        if(status == PARSING_ERROR){
            printf("error");
            return NULL;
        }
        if(status == NO_END_OF_LINE){
            continue;
        }
        if(status == OK){
            vheader_t_push_back(&req.headers, &header);
        }
        if(status == END_OF_HEADER) {
            if (host_fd == -1) {
                host_fd = socket(AF_INET, SOCK_STREAM, 0);
                ASSERT(host_fd >= 0);

                header_t *host_name = find_header(&req.headers, "Host");
                ASSERT(host_name != NULL);

                struct sockaddr_in server_addr;
                name2addr(host_name->value, 80, &server_addr);
                ASSERT(connect(host_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) == 0);
            }

            
        }

        break;
    }
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

    request_t req;
    request_init(&req);
    const char* cur = buffer;
    parse_req_type(&cur, &req);

    while(1){
        header_t header;
        int status = parse_next_header(&cur, &header);

        if(status == PARSING_ERROR || status == NO_END_OF_LINE){
            printf("bruuuh");
            exit(1);
        }

        vheader_t_push_back(&req.headers, &header);
        if(status == END_OF_HEADER){
            break;
        }
    }

    for(size_t i=0; i < req.headers.cnt; ++i){
        header_t* header = vheader_t_get(&req.headers, i);
        printf(" - %s :: %s\n", header->type, header->value);
    }

    close(fd);
    close_servsock(&servsock);

    return 0;
}
