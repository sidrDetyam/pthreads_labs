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
name2addr(const char *host_name, uint16_t host_port, struct sockaddr_in *sock_addr) {
    struct hostent *server = gethostbyname(host_name);
    ASSERT_RETURN(server != NULL);

    memset(&sock_addr->sin_zero, 0, sizeof(sock_addr->sin_zero));
    sock_addr->sin_family = AF_INET;
    sock_addr->sin_port = htons(host_port);
    memcpy(&sock_addr->sin_addr.s_addr, server->h_addr_list[0], server->h_length);

    return SUCCESS;
}


struct ConnectionHandlerContext {
    int fd;
};
typedef struct ConnectionHandlerContext context_t;


int
read_req(request_t *req, int fd, char **ppos_, char **rpos_) {
    request_init(req);

    char *rpos = *rpos_;
    const char *ppos = *ppos_;
    while (1) {
        ssize_t cnt = read(fd, rpos, 100000);
        rpos += cnt;
        if (cnt == -1 || cnt == 0) {
            return ERROR;
        }

        int status = parse_req_type(&ppos, req);
        if (status == NO_END_OF_LINE) {
            continue;
        }
        if (status == OK) {
            break;
        }
        if (status == PARSING_ERROR) {
            printf("error\n");
            return ERROR;
        }
    }

    int isf = 0;
    while (1) {
        if (isf) {
            ssize_t cnt = read(fd, rpos, 100000);
            rpos += cnt;
            isf = 0;
        }

        header_t header;
        int status = parse_next_header(&ppos, &header);
        if (status == PARSING_ERROR) {
            printf("error");
            return ERROR;
        }
        if (status == NO_END_OF_LINE) {
            isf = 1;
            continue;
        }
        if (status == OK) {
            vheader_t_push_back(&req->headers, &header);
        }
        if (status == END_OF_HEADER) {
            ASSERT(strcmp(req->type, "GET") == 0);
            *ppos_ = (char *) ppos;
            *rpos_ = (char *) rpos;
            return SUCCESS;
        }
    }
}

int
read_response(response_t *response, int fd, char **ppos_, char **rpos_) {
    response_init(response);

    char *rpos = *rpos_;
    const char *ppos = *ppos_;
    while (1) {
        ssize_t cnt = read(fd, rpos, 100000);
        rpos += cnt;
        if (cnt == -1 || cnt == 0) {
            return ERROR;
        }

        int status = parse_response_code(&ppos, response);
        if (status == NO_END_OF_LINE) {
            continue;
        }
        if (status == OK) {
            break;
        }
        if (status == PARSING_ERROR) {
            printf("error\n");
            return ERROR;
        }
    }

    int isf = 0;
    const char *end_of_headers;
    size_t contlen;
    while (1) {
        if (isf) {
            ssize_t cnt = read(fd, rpos, 100000);
            rpos += cnt;
            isf = 0;
        }

        header_t header;
        int status = parse_next_header(&ppos, &header);
        if (status == PARSING_ERROR) {
            printf("error");
            return ERROR;
        }
        if (status == NO_END_OF_LINE) {
            isf = 1;
            continue;
        }
        if (status == OK) {
            vheader_t_push_back(&response->headers, &header);
        }
        if (status == END_OF_HEADER) {
            header_t *h = find_header(&response->headers, "Content-Length");
            ASSERT(h != NULL);
            contlen = (size_t) atoi(h->value);
            break;
        }
    }

    size_t cc = rpos - ppos;
    while (rpos - ppos < contlen) {
        ssize_t cnt = read(fd, rpos, 100000);
        rpos += cnt;
        if (cnt == 0 || cnt == -1) {
            return ERROR;
        }
    }

    *ppos_ = (char *) ppos;
    *rpos_ = (char *) rpos;
    return SUCCESS;
}


void write_n(int fd, const char* buff, size_t size){
    size_t writen = 0;
    while(writen < size){
        ssize_t cnt = write(fd, buff, size - writen);
        ASSERT(cnt != -1);
        writen += cnt;
    }
}


void *
handler(void *arg) {
    context_t *context = (context_t *) arg;

    char *client_buff = malloc(1000000);
    char *server_buff = malloc(1000000);

    char *crpos = client_buff;
    char *cppos = client_buff;

    char *srpos = server_buff;
    char *sppos = server_buff;

    request_t req;
    response_t response;

    int st = read_req(&req, context->fd, &cppos, &crpos);
    if (st == ERROR) {
        printf("ERROR\n");
        return NULL;
    }

    int host_fd = socket(AF_INET, SOCK_STREAM, 0);
    ASSERT(host_fd >= 0);

    header_t *host_name = find_header(&req.headers, "Host");
    ASSERT(host_name != NULL);

    struct sockaddr_in server_addr;
    name2addr(host_name->value, 80, &server_addr);
    ASSERT(connect(host_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) == 0);

    write_n(host_fd, client_buff, cppos - client_buff);

    st = read_response(&response, host_fd, &sppos, &srpos);
    if (st == ERROR) {
        printf("ERROR\n");
        return NULL;
    }

    size_t cc = srpos - server_buff;
    write_n(context->fd, server_buff, cc);

    return NULL;
}


int main() {

    char *buffer = malloc(100000);
    memset(buffer, 0, 100000);
    char *hello = "Hello from server";

    servsock_t servsock;
    ASSERT(create_servsock(8080, 3, &servsock) == SUCCESS);
    int fd;
    ASSERT((fd = accept_servsock(&servsock)) != ERROR);

    context_t context = {fd};
    handler(&context);

//    read(fd, buffer, 100000);
//    printf("%s\n", buffer);
//
//    request_t req;
//    request_init(&req);
//    const char *cur = buffer;
//    parse_req_type(&cur, &req);
//
//    while (1) {
//        header_t header;
//        int status = parse_next_header(&cur, &header);
//
//        if (status == PARSING_ERROR || status == NO_END_OF_LINE) {
//            printf("bruuuh");
//            exit(1);
//        }
//
//        vheader_t_push_back(&req.headers, &header);
//        if (status == END_OF_HEADER) {
//            break;
//        }
//    }
//
//    for (size_t i = 0; i < req.headers.cnt; ++i) {
//        header_t *header = vheader_t_get(&req.headers, i);
//        printf(" - %s :: %s\n", header->type, header->value);
//    }

    close(fd);
    close_servsock(&servsock);

    return 0;
}