//
// Created by argem on 03.12.2022.
//

#include "connection_handler.h"

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

#define ELEMENT_TYPE char

#include "cvector_impl.h"


int
name2addr(const char *host_name, uint16_t host_port, struct sockaddr_in *sock_addr) {
    struct hostent *server = gethostbyname(host_name);
    ASSERT_RETURN(server != NULL);

    memset(&sock_addr->sin_zero, 0, sizeof(sock_addr->sin_zero));
    sock_addr->sin_family = AF_INET;
    sock_addr->sin_port = htons(host_port);
    memcpy(&sock_addr->sin_addr.s_addr, server->h_addr_list[0], server->h_length);

    return SUCCESS;
}


void init_context(handler_context_t *context, int client_fd) {
    request_init(&context->request);
    response_init(&context->response);

    vchar_init(&context->cbuff);
    vchar_forced_alloc(&context->cbuff);
    context->cppos = context->cbuff.ptr;
    //context->crpos = context->cbuff.ptr;

    vchar_init(&context->sbuff);
    vchar_forced_alloc(&context->sbuff);
    context->sppos = context->sbuff.ptr;
    //context->srpos = context->sbuff.ptr;

    context->sended = 0;

    context->client_fd = client_fd;
    context->server_fd = -1;
    context->client_events = POLLIN;
    context->server_events = 0;
    context->handling_step = PARSING_REQ_TYPE;
}

void
destroy_context(handler_context_t *context) {
    request_destroy(&context->request);
    response_destroy(&context->response);
    vchar_free(&context->cbuff);
    vchar_free(&context->sbuff);

    close(context->client_fd);
    if (context->server_fd != -1) {
        close(context->server_fd);
    }

    context->handling_step = HANDLER_EXCEPTIONALLY;
}


enum Config {
    MIN_READ_BUFF_SIZE = 100,
    DEFAULT_PORT = 80
};


static int
read_to_vchar(int fd, vchar *buff, size_t *read_) {
    vchar_alloc2(buff, MIN_READ_BUFF_SIZE + 1);
    ssize_t cnt = read(fd, buff->ptr, MIN_READ_BUFF_SIZE);
    if (cnt == -1) {
        return ERROR;
    }
    if(read_ != NULL){
        *read_ = cnt;
    }
    buff->cnt += cnt;
    buff->ptr += cnt;
    *buff->ptr = '\0';
    return SUCCESS;
}


static void
parsing_req_type_step(handler_context_t *context, int fd, int events) {
    ASSERT(context->handling_step == PARSING_REQ_TYPE &&
           fd == context->client_fd && (events & POLLIN));

    ASSERT_RETURN2_C(read_to_vchar(context->client_fd, &context->cbuff, NULL) == SUCCESS,
                     destroy_context(context),);

    int status = parse_req_type((const char **) &context->cppos, &context->request);

    ASSERT_RETURN2_C(status != PARSING_ERROR,
                     destroy_context(context),);

    if (status == OK) {
        context->handling_step = PARSING_REQ_HEADERS;
    }
}

static void
parsing_req_headers_step(handler_context_t *context, int fd, int events){
    ASSERT(context->handling_step == PARSING_REQ_HEADERS &&
           fd == context->client_fd && (events & POLLIN));

    ASSERT_RETURN2_C(read_to_vchar(context->client_fd, &context->cbuff, NULL) == SUCCESS,
                     destroy_context(context),);

    while(1){
        header_t header;
        int status = parse_next_header((const char **) &context->cppos, &header);
        ASSERT_RETURN2_C(status != PARSING_ERROR,
                         destroy_context(context),);

        if(status == NO_END_OF_LINE){
            return;
        }
        if(status == OK){
            vheader_t_push_back(&context->request.headers, &header);
            continue;
        }
        if(status == END_OF_HEADER){
            ASSERT_RETURN2_C(strcmp(context->request.type, "GET") == 0,
                             destroy_context(context),);

            ASSERT(context->server_fd == -1);
            ASSERT_RETURN2_C((context->server_fd = socket(AF_INET, SOCK_STREAM, 0)) >= 0,
                             destroy_context(context),);

            header_t *host_name = find_header(&context->request.headers, "Host");
            ASSERT_RETURN2_C(host_name != NULL,
                             destroy_context(context),);

            struct sockaddr_in host_addr;
            name2addr(host_name->value, DEFAULT_PORT, &host_addr);
            ASSERT_RETURN2_C(connect(context->server_fd,
                                     (struct sockaddr *) &host_addr, sizeof(host_addr))==0,
                             destroy_context(context),);

            context->client_events = 0;
            context->server_events = POLLOUT;
            context->sended = 0;
            context->handling_step = SENDING_REQ;
            return;
        }
    }
}


static void
sending_req_step(handler_context_t* context, int fd, int events){
    ASSERT(context->handling_step == SENDING_REQ &&
           fd == context->server_fd && (events & POLLOUT));

    ssize_t cnt = write(context->server_fd, context->cbuff.ptr + context->sended,
                        context->cbuff.cnt - context->sended);
    ASSERT_RETURN2_C(cnt != -1, destroy_context(context),);
    context->sended += cnt;

    if(context->sended == context->cbuff.cnt){
        context->client_events = 0;
        context->server_events = POLLIN;
        context->handling_step = PARSING_RESP_CODE;
    }
}


static void
parsing_resp_code_step(handler_context_t* context, int fd, int events){
    ASSERT(context->handling_step == PARSING_RESP_CODE &&
           fd == context->server_fd && (events & POLLIN));

    ASSERT_RETURN2_C(read_to_vchar(context->server_fd, &context->sbuff, NULL) == SUCCESS,
                     destroy_context(context),);

    int status = parse_response_code((const char **) &context->sppos, &context->response);

    ASSERT_RETURN2_C(status != PARSING_ERROR,
                     destroy_context(context),);

    if (status == OK) {
        context->handling_step = PARSING_RESP_HEADERS;
    }
}


static void
parsing_resp_headers_step(handler_context_t* context, int fd, int events){
    ASSERT(context->handling_step == PARSING_RESP_HEADERS &&
           fd == context->server_fd && (events & POLLIN));

    ASSERT_RETURN2_C(read_to_vchar(context->server_fd, &context->sbuff, NULL) == SUCCESS,
                     destroy_context(context),);

    while(1){
        header_t header;
        int status = parse_next_header((const char **) &context->sppos, &header);
        ASSERT_RETURN2_C(status != PARSING_ERROR,
                         destroy_context(context),);

        if(status == NO_END_OF_LINE){
            return;
        }
        if(status == OK){
            vheader_t_push_back(&context->response.headers, &header);
            continue;
        }
        if(status == END_OF_HEADER){
            header_t *cl_header = find_header(&context->response.headers, "Content-Length");
            header_t *ch_header = find_header(&context->response.headers, "Transfer-Encoding");
            ASSERT_RETURN2_C(ch_header != NULL || cl_header != NULL,
                             destroy_context(context),);

            context->response.content_length = cl_header!=NULL? (size_t) atoi(cl_header->value) : -1;
            context->read_ = 0;
            context->handling_step = PARSING_RESP_BODY;
            return;
        }
    }
}


static void
parsing_resp_body(handler_context_t* context, int fd, int events){
    ASSERT(context->handling_step == PARSING_RESP_BODY &&
           fd == context->server_fd && (events & POLLIN));

    size_t read_;
    ASSERT_RETURN2_C(read_to_vchar(context->server_fd, &context->sbuff, &read_) == SUCCESS,
                     destroy_context(context),);

    context->read_ += read_;

    if(context->response.content_length != -1
        && context->response.content_length == context->read_){
        context->client_events = POLLOUT;
        context->server_events = 0;
        context->sppos = context->sbuff.ptr;
        context->handling_step = SENDING_RESP;
        return;
    }

    if(context->response.content_length == -1){
        char *crlf = strstr(ppos, "\r\n");
        if (crln == NULL) {
                while (1) {
                    ssize_t tmp = read(fd, rpos, 1000000);
                    printf("read in chunk %zu\n", tmp);
                    if (tmp == -1) {
                        return ERROR;
                    }
                    rpos += tmp;
                    if (tmp > 0) {
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

            if (chunkSize == 0){
                while(1) {
                    header_t trailer;
                    int status = parse_next_header(&ppos, &trailer);
                    if (status == OK) {
                        continue;
                    }
                    if(status == END_OF_HEADER){
                        break;
                    }
                }
                break;
            }

            size_t nn = rpos - ppos;
            if (nn < chunkSize + 2) {
                read_n(fd, rpos, chunkSize + 2 - nn);
                printf("read in chunk\n");
                rpos += chunkSize + 2 - (rpos - ppos);
            }
            ppos += chunkSize + 2;
    }
}


void
handle(handler_context_t *context, int fd, int events) {
    if (context->handling_step == PARSING_REQ_TYPE) {
        if ()
    }
}


