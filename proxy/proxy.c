#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <poll.h>
#include <limits.h>
#include <pthread.h>


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


void write_n(int fd, const char *buff, size_t size) {
    size_t writen = 0;
    while (writen < size) {
        ssize_t cnt = write(fd, buff, size - writen);
        ASSERT(cnt != -1);
        writen += cnt;
    }
    ASSERT(writen == size);
}

void read_n(int fd, char *buff, size_t size) {
    size_t readen = 0;
    while (readen < size) {
        ssize_t cnt = read(fd, buff, size - readen);
        ASSERT(cnt != -1);
        readen += cnt;
    }
    ASSERT(readen == size);
}

size_t read_while(int fd, char *buff) {
    size_t readen = 0;
    while (1) {
        ssize_t cnt = read(fd, buff, 1);
        readen += 1;
        ASSERT(cnt != -1);
        if (cnt == 0) {
            return readen;
        }
    }
}


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
            //ASSERT(strcmp(req->type, "GET") == 0);
            if(strcmp(req->type, "GET") != 0){
                return ERROR;
            }
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
        sleep(1);
        ssize_t cnt = read(fd, rpos, 1000000);
        printf("%zd\n", cnt);
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
    int is_chunked = 0;
    size_t contlen = 0;
    while (1) {
        if (isf) {
            ssize_t cnt = read(fd, rpos, 1000000);
            rpos += cnt;
            if(cnt == -1){
                return ERROR;
            }
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
            header_t *cl = find_header(&response->headers, "Content-Length");
            header_t *ch = find_header(&response->headers, "Transfer-Encoding");
            //ASSERT(cl != NULL || ch != NULL);
            if(cl == NULL && ch == NULL){
                return ERROR;
            }

            if (cl != NULL) {
                contlen = (size_t) atoi(cl->value);
            } else {
                is_chunked = 1;
            }
            break;
        }
    }

    if (is_chunked) {
        while (1) {
            char *crln = strstr(ppos, "\r\n");
            if(crln == NULL){
                while(1){
                    ssize_t tmp = read(fd, rpos, 1000000);
                    if(tmp == -1){
                        return ERROR;
                    }
                    rpos += tmp;
                    if(tmp > 0){
                        break;
                    }
                }
                continue;
            }

            char num[100];
            memcpy(num, ppos, crln - ppos);
            num[crln - ppos] = '\0';
            long chunkSize = strtol(num, NULL, 16);
            ppos = crln + 2;

            size_t nn = rpos - ppos;
            if (nn < chunkSize + 2) {
                read_n(fd, rpos, chunkSize + 2 - nn);
                rpos += chunkSize + 2 - (rpos - ppos);
            }
            ppos += chunkSize + 2;

            if (chunkSize == 0) {
                break;
            }
        }
    } else {
        size_t h_cc = ppos - *ppos_;
        size_t cc = rpos - ppos;
        sleep(1);
        read_n(fd, rpos, contlen - cc);
        ppos += contlen;
        rpos +=contlen - cc;

//        while (rpos - ppos < contlen) {
//            ssize_t cnt = read(fd, rpos, 100000);
//            rpos += cnt;
//            if (cnt == -1) {
//                return ERROR;
//            }
//        }
    }

    *ppos_ = (char *) ppos;
    *rpos_ = (char *) rpos;
    return SUCCESS;
}


void *
handler(void *arg) {
    context_t *context = (context_t *) arg;

    char *client_buff = malloc(30000000);
    char *server_buff = malloc(30000000);

    char *crpos = client_buff;
    char *srpos = server_buff;

    char *cppos = crpos;
    char *sppos = srpos;

    int host_fd = -1;
    while (1) {

        memset(client_buff, 0, 30000000);
        memset(server_buff, 0, 30000000);

        char* sst = sppos;
        char* cst = cppos;

        request_t req;
        response_t response;

        req.type = "";
        req.uri = "";
        int st = read_req(&req, context->fd, &cppos, &crpos);
        if (st == ERROR) {
            printf("req ERROR %s %s\n", req.uri, req.type);
            if(strcmp(req.type, "GET")==0){
                printf("\n");
            }

            if(host_fd != -1){
                close(host_fd);
            }
            close(context->fd);
            free(server_buff);
            free(client_buff);
            return NULL;
        }

        int attemps = 3;
        while(1) {
            if (host_fd == -1) {
                host_fd = socket(AF_INET, SOCK_STREAM, 0);
                ASSERT(host_fd >= 0);
                header_t *host_name = find_header(&req.headers, "Host");
                ASSERT(host_name != NULL);
                struct sockaddr_in server_addr;
                name2addr(host_name->value, 80, &server_addr);
                ASSERT(connect(host_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) == 0);
            }

            write_n(host_fd, cst, cppos - cst);

            st = read_response(&response, host_fd, &sppos, &srpos);
            if (st == ERROR) {
                printf("response ERROR\n");
                close(host_fd);
                attemps--;
                if(attemps == 0){
                    close(host_fd);
                    close(context->fd);
                    free(server_buff);
                    free(client_buff);
                    return NULL;
                }
                host_fd = -1;
                continue;
            }

            break;
        }

        size_t cc = sppos - sst;
        write_n(context->fd, sst, cc);
//

        shutdown(context->fd, SHUT_RDWR);
        close(context->fd);
        shutdown(host_fd, SHUT_RDWR);
        close(host_fd);
        return NULL;
    }
    return NULL;
}


int main() {

    char *buffer = malloc(100000);
    memset(buffer, 0, 100000);
    char *hello = "Hello from server";

    servsock_t servsock;
    ASSERT(create_servsock(8080, 10, &servsock) == SUCCESS);
    int fd;

    int i = 0;
    while (1) {
        ASSERT((fd = accept_servsock(&servsock)) != ERROR);
        context_t* context = malloc(sizeof(context_t));
        context->fd = fd;

        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 250;
        if (setsockopt (fd, SOL_SOCKET, SO_RCVTIMEO, &timeout,
                        sizeof(timeout)) < 0){
            perror("timeout");
            exit(1);
        }

        pthread_t *t = malloc(sizeof(pthread_t));
//        handler(context);
        printf("%d connetct!!\n", i);
        ++i;
        handler(context);
        //pthread_create(t, NULL, handler, context);
    }

    close(fd);
    close_servsock(&servsock);

    return 0;
}
