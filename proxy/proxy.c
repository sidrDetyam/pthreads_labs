#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <poll.h>
#include "socket_utils.h"
#include "common.h"

#include "connection_handler.h"
#include "hash_map.h"


int find_context(handler_context_t *context1, size_t cnt, int fd) {
    for (size_t i = 0; i < cnt; ++i) {
        if (context1[i].client_fd == fd || context1[i].server_fd == fd) {
            return (int) i;
        }
    }

    return -1;
}


int request_hash(void* req){
    return 1;
}

int request_equals(void* req1_, void* req2_){
    request_t *req1 = req1_;
    request_t *req2 = req2_;

    header_t* host1 = find_header(&req1->headers, "Host");
    header_t* host2 = find_header(&req2->headers, "Host");
    if(host1 == NULL || host2 == NULL){
        return 0;
    }

    return strcmp(host1->value, host2->value)==0 && strcmp(req1->type, req2->type)==0
    && strcmp(req1->version, req2->version)==0 && strcmp(req1->uri, req2->uri)==0;
}




int main() {
    servsock_t servsock;
    ASSERT(create_servsock(4242, 100, &servsock) == SUCCESS);

    struct pollfd fds[200];

    handler_context_t *context1 = malloc(sizeof(handler_context_t) * 100);
    size_t contexts_count = 0;

    fds[0].fd = servsock.fd;
    fds[0].events = POLLIN;
    size_t fds_count = 1;

    hash_map_t hm;
    hash_map_init(&hm, sizeof(request_t), sizeof(vchar), request_hash, request_equals);


    int _i = 0;
    while (1) {
        ASSERT(poll(fds, fds_count, 1000) != -1);
        //printf("here %d %zu\n", _i++, fds_count);

        if (fds[0].revents & POLLIN) {
            int new_fd;
            ASSERT((new_fd = accept_servsock(&servsock, 0)) != ERROR);

            struct timeval timeout;
            timeout.tv_sec = 0;
            timeout.tv_usec = 250000;
            if (setsockopt(new_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout,
                           sizeof(timeout)) < 0) {
                perror("timeout");
                exit(1);
            }

            init_context(context1 + contexts_count, new_fd, &hm);
            ++contexts_count;
            printf("connect %zu\n", contexts_count);
        }
        for (size_t i = 1; i < fds_count; ++i) {
            int ind = find_context(context1, contexts_count, fds[i].fd);
            ASSERT(ind != -1);
            if (context1[ind].client_fd == fds[i].fd && (context1[ind].client_events & fds[i].revents)
                || context1[ind].server_fd == fds[i].fd && (context1[ind].server_events & fds[i].revents)) {

                handle(context1 + ind, fds[i].fd, fds[i].revents);
                if (context1[ind].handling_step == HANDLED || context1[ind].handling_step == HANDLED_EXCEPTIONALLY) {
                    for (int j = ind; j < contexts_count; ++j) {
                        memcpy(context1 + j, context1 + j + 1, sizeof(handler_context_t));
                    }
                    --contexts_count;
                    printf("disconnect %zu\n", contexts_count);
                }
            }
        }
        fds[0].fd = servsock.fd;
        fds_count = 1;
        for (size_t i = 0; i < contexts_count; ++i) {
            if (context1[i].client_events != 0) {
                fds[fds_count].fd = context1[i].client_fd;
                fds[fds_count].events = (short) context1[i].client_events;
                ++fds_count;
            }
            if (context1[i].server_events != 0 && context1[i].server_fd != -1) {
                fds[fds_count].fd = context1[i].server_fd;
                fds[fds_count].events = (short) context1[i].server_events;
                ++fds_count;
            }
        }
    }
}
