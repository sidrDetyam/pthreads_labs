//
// Created by argem on 03.12.2022.
//

#include "connection_handler.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <poll.h>
#include "common.h"
#include "socket_utils.h"

#define ELEMENT_TYPE char

#include "cvector_impl.h"

//#define ELEMENT_TYPE handler_context_t;
//#include "cvector_impl.h"


void init_context(handler_context_t *context, int client_fd) {
    request_init(&context->request);
    response_init(&context->response);

    vchar_init(&context->cbuff);
    vchar_forced_alloc(&context->cbuff);
    context->cppos = 0;
    vchar_init(&context->sbuff);
    vchar_forced_alloc(&context->sbuff);
    context->sppos = 0;
    context->sended = 0;

    context->connection_state = NOT_CONNECTED;
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

    context->handling_step = HANDLED_EXCEPTIONALLY;
}


enum Config {
    MIN_READ_BUFF_SIZE = 10000,
    DEFAULT_PORT = 80,
    //RECV_TIMEOUT_US = 250000,
    //KEEP_ALIVE_SUPPORT = 1
};
#define KEEP_ALIVE_SUPPORT 0


static int
read_to_vchar(int fd, vchar *buff, size_t *read_) {
    vchar_alloc2(buff, MIN_READ_BUFF_SIZE + 1);
    ssize_t cnt = read(fd, &buff->ptr[buff->cnt], MIN_READ_BUFF_SIZE);
    if (cnt == -1 || cnt == 0) {
        return ERROR;
    }
    if (read_ != NULL) {
        *read_ = cnt;
    }
    buff->cnt += cnt;
    buff->ptr[buff->cnt] = '\0';
    return SUCCESS;
}


static void
parsing_req_type_step(handler_context_t *context, int fd, int events, int non_splitted) {
    if(!non_splitted) {
        ASSERT(context->handling_step == PARSING_REQ_TYPE &&
               fd == context->client_fd && (events & POLLIN));
        ASSERT_RETURN2_C(read_to_vchar(context->client_fd, &context->cbuff, NULL) == SUCCESS,
                         destroy_context(context),);
    }

    const char *cppos = context->cbuff.ptr + context->cppos;
    int status = parse_req_type(&cppos, &context->request);
    context->cppos = cppos - context->cbuff.ptr;

    ASSERT_RETURN2_C(status != PARSING_ERROR,
                     destroy_context(context),);

    if (status == OK) {
        context->handling_step = PARSING_REQ_HEADERS;
    }
}


static void
connect_step(handler_context_t *context, int fd, int events);


static void
parsing_req_headers_step(handler_context_t *context, int fd, int events, int non_splitted) {
    ASSERT(context->handling_step == PARSING_REQ_HEADERS &&
           fd == context->client_fd && (events & POLLIN));

    if (!non_splitted) {
        ASSERT_RETURN2_C(read_to_vchar(context->client_fd, &context->cbuff, NULL) == SUCCESS,
                         destroy_context(context),);
    }

    while (1) {
        header_t header;
        const char *cppos = context->cbuff.ptr + context->cppos;
        int status = parse_next_header(&cppos, &header);
        context->cppos = cppos - context->cbuff.ptr;

        ASSERT_RETURN2_C(status != PARSING_ERROR,
                         destroy_context(context),);

        if (status == NO_END_OF_LINE) {
            return;
        }
        if (status == OK) {
            vheader_t_push_back(&context->request.headers, &header);
            continue;
        }
        if (status == END_OF_HEADER) {
            ASSERT_RETURN2_C(strcmp(context->request.type, "GET") == 0,
                             destroy_context(context),);

            context->handling_step = CONNECT_STEP;
            connect_step(context, -1, 0);
            return;
        }
    }
}


static void
connect_step(handler_context_t *context, int fd, int events) {
    if (context->connection_state == AWAIT_CONNECTION) {
        ASSERT(context->handling_step == CONNECT_STEP &&
               fd == context->server_fd && (events & POLLOUT));
    }

    if (context->connection_state == CONNECTED || context->connection_state == AWAIT_CONNECTION) {
        context->client_events = 0;
        context->server_events = POLLOUT;
        context->sended = 0;
        context->handling_step = SENDING_REQ;

        if (context->connection_state == AWAIT_CONNECTION) {
            int connection_result;
            socklen_t _ = sizeof(connection_result);
            ASSERT_RETURN2_C(getsockopt(context->server_fd, SOL_SOCKET, SO_ERROR, &connection_result, &_) >= 0
                             && connection_result == 0,
                             destroy_context(context),);
            context->connection_state = CONNECTED;
        }
        return;
    }

    ASSERT_RETURN2_C((context->server_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0)) >= 0,
                     destroy_context(context),);

    header_t *host_name = find_header(&context->request.headers, "Host");
    ASSERT_RETURN2_C(host_name != NULL,
                     destroy_context(context),);

    int rc = connect_to_host(context->server_fd, host_name->value, DEFAULT_PORT);
    if (rc == 0) {
        context->connection_state = CONNECTED;
    } else if (errno == EINPROGRESS) {
        context->client_events = 0;
        context->server_events = POLLOUT;
        context->connection_state = AWAIT_CONNECTION;
    } else {
        destroy_context(context);
    }
}


static void
sending_req_step(handler_context_t *context, int fd, int events) {
    ASSERT(context->handling_step == SENDING_REQ &&
           fd == context->server_fd && (events & POLLOUT));

    ssize_t cnt = write(context->server_fd, context->cbuff.ptr + context->sended,
                        context->cbuff.cnt - context->sended);
    ASSERT_RETURN2_C(cnt > 0, destroy_context(context),);
    context->sended += cnt;

    if (context->sended == context->cbuff.cnt) {
        context->client_events = 0;
        context->server_events = POLLIN;
        context->handling_step = PARSING_RESP_CODE;
    }
}


static void
parsing_resp_code_step(handler_context_t *context, int fd, int events) {
    ASSERT(context->handling_step == PARSING_RESP_CODE &&
           fd == context->server_fd && (events & POLLIN));

    ASSERT_RETURN2_C(read_to_vchar(context->server_fd, &context->sbuff, NULL) == SUCCESS,
                     destroy_context(context),);

    const char *sppos = context->sbuff.ptr + context->sppos;
    int status = parse_response_code(&sppos, &context->response);
    context->sppos = sppos - context->sbuff.ptr;

    ASSERT_RETURN2_C(status != PARSING_ERROR,
                     destroy_context(context),);

    if (status == OK) {
        context->handling_step = PARSING_RESP_HEADERS;
    }
}


static void
parsing_resp_headers_step(handler_context_t *context, int fd, int events, int non_splitted) {
    ASSERT(context->handling_step == PARSING_RESP_HEADERS &&
           fd == context->server_fd && (events & POLLIN));

    if (!non_splitted) {
        ASSERT_RETURN2_C(read_to_vchar(context->server_fd, &context->sbuff, NULL) == SUCCESS,
                         destroy_context(context),);
    }

    while (1) {
        header_t header;
        const char *sppos = context->sbuff.ptr + context->sppos;
        int status = parse_next_header(&sppos, &header);
        context->sppos = sppos - context->sbuff.ptr;

        ASSERT_RETURN2_C(status != PARSING_ERROR,
                         destroy_context(context),);

        if (status == NO_END_OF_LINE) {
            return;
        }
        if (status == OK) {
            vheader_t_push_back(&context->response.headers, &header);
            continue;
        }
        if (status == END_OF_HEADER) {
            header_t *cl_header = find_header(&context->response.headers, "Content-Length");
            header_t *ch_header = find_header(&context->response.headers, "Transfer-Encoding");
            ASSERT_RETURN2_C(ch_header != NULL || cl_header != NULL,
                             destroy_context(context),);

            context->response.content_length = cl_header != NULL ?
                                               (long) strtol(cl_header->value, NULL, 10) : -1;
            context->read_ = context->sbuff.cnt - context->sppos;
            context->chunk_size = -1;
            context->chunk_read = 0;
            context->client_events = POLLOUT;
            context->sended = 0;
            context->handling_step = PARSING_RESP_BODY;
            return;
        }
    }
}


static void
parsing_resp_body(handler_context_t *context, int fd, int events, int non_splitted) {
    ASSERT(context->handling_step == PARSING_RESP_BODY &&
           fd == context->server_fd && (events & POLLIN));

    if (!non_splitted) {
        size_t read_;
        ASSERT_RETURN2_C(read_to_vchar(context->server_fd, &context->sbuff, &read_) == SUCCESS,
                         destroy_context(context),);
        context->read_ += read_;
    }

    if (context->response.content_length != -1
        && context->response.content_length == context->read_) {
        context->client_events = POLLOUT;
        context->server_events = 0;
        context->sppos = context->sbuff.cnt;
        context->handling_step = SENDING_RESP;
        return;
    }

    if (context->response.content_length == -1) {
        while (1) {
            if (context->chunk_size == -1) {
                char *sppos = context->sbuff.ptr + context->sppos;
                char *crlf = strstr(sppos, "\r\n");
                if (crlf == NULL) {
                    return;
                }
                context->chunk_size = strtol(sppos, NULL, 16) + 2;
                context->chunk_read = 0;
                context->sppos += crlf - sppos + 2;
            }
            size_t ra = MIN(context->chunk_size - context->chunk_read,
                            context->sbuff.cnt - context->sppos);

            context->sppos += ra;
            context->chunk_read += ra;
            if (context->chunk_read == context->chunk_size) {
                if (context->chunk_size == 2) {
                    context->client_events = POLLOUT;
                    context->server_events = 0;
                    context->response.content_length = (long) context->read_;
                    ASSERT((context->response.body = malloc(context->read_ + 1)) != NULL);
                    memcpy(context->response.body,
                           context->sbuff.ptr + context->sbuff.cnt - context->read_, context->read_);
                    context->handling_step = SENDING_RESP;
                    return;
                }

                context->chunk_read = 0;
                context->chunk_size = -1;
                continue;
            }
            return;
        }
    }
}


static void
send_resp_in_receiving(handler_context_t *context, int fd, int events) {
    ASSERT(context->handling_step == PARSING_RESP_BODY || context->handling_step == SENDING_RESP &&
                                                          fd == context->client_fd && (events & POLLOUT));

    if (context->sbuff.cnt - context->sended > 0) {
        ssize_t cnt = write(context->client_fd, context->sbuff.ptr + context->sended,
                            context->sbuff.cnt - context->sended);
        ASSERT_RETURN2_C(cnt > 0, destroy_context(context),);
        context->sended += cnt;
    }
}


static void
send_resp_step(handler_context_t *context, int fd, int events) {
    send_resp_in_receiving(context, fd, events);

    if (context->sended == context->sbuff.cnt) {
        header_t *connection = find_header(&context->response.headers, "Connection");
        if (connection != NULL && strcasecmp(connection->value, "Keep-Alive") == 0 && KEEP_ALIVE_SUPPORT) {
            request_destroy(&context->request);
            response_destroy(&context->response);
            request_init(&context->request);
            response_init(&context->response);

            for(size_t i=context->cppos; i<context->cbuff.cnt; ++i){
                context->cbuff.ptr[i - context->cppos] = context->cbuff.ptr[i];
            }
            context->cbuff.cnt -= context->cppos;
            context->cbuff.ptr[context->cbuff.cnt] = '\0';
            context->cppos = 0;

            context->sbuff.cnt = 0;
            context->sppos = 0;
            context->client_events = POLLIN;
            context->server_events = 0;
            context->handling_step = PARSING_REQ_TYPE;

            parsing_req_type_step(context, -1, 0, 1);
            return;
        }
        destroy_context(context);
        context->handling_step = HANDLED;
    }
}


void
handle(handler_context_t *context, int fd, int events) {
    int non_splitted = 0;

    if (context->handling_step == PARSING_REQ_TYPE) {
        parsing_req_type_step(context, fd, events, non_splitted);
        non_splitted = 1;
    }
    if (context->handling_step == PARSING_REQ_HEADERS) {
        parsing_req_headers_step(context, fd, events, non_splitted);
        return;
    }
    if (context->handling_step == CONNECT_STEP) {
        connect_step(context, fd, events);
        return;
    }
    if (context->handling_step == SENDING_REQ) {
        sending_req_step(context, fd, events);
        return;
    }
    if (context->handling_step == PARSING_RESP_CODE) {
        parsing_resp_code_step(context, fd, events);
        non_splitted = 1;
    }
    if (context->handling_step == PARSING_RESP_HEADERS) {
        parsing_resp_headers_step(context, fd, events, non_splitted);
        non_splitted = 1;
    }
    if (context->handling_step == PARSING_RESP_BODY) {
        if (fd == context->server_fd) {
            parsing_resp_body(context, fd, events, non_splitted);
        }
        if (fd == context->client_fd) {
            send_resp_in_receiving(context, fd, events);
        }
        return;
    }
    if (context->handling_step == SENDING_RESP) {
        send_resp_step(context, fd, events);
    }
}
