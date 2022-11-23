#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <poll.h>

#include "../common.h"

static void
name2addr(const char* host_name, uint16_t host_port, struct sockaddr_in* sock_addr){
    struct hostent *server = gethostbyname(host_name);
    ASSERT(server != NULL);
    memset(&sock_addr->sin_zero, 0, sizeof(sock_addr->sin_zero));
    sock_addr->sin_family = AF_INET;
    sock_addr->sin_port = htons(host_port);
    memcpy(&sock_addr->sin_addr.s_addr,server->h_addr_list[0], server->h_length);
}

enum CONFIG{
    IN_BUFFER_SIZE = 1000000,
    MX_SCREEN_CHARS = 100
};

static void
write_(const char* buff, size_t buff_size, int fd){
    for(size_t written = 0; written < buff_size; ){
        ssize_t cnt = write(fd,buff+written, buff_size-written);
        written += cnt;
        ASSERT(cnt >= 0);
    }
}


int
main(int argc, __attribute__((unused)) char *argv[]){

    //ASSERT(argc == 3);
    const char* host_name = "parallels.nsu.ru";//argv[1];
    const int host_port = 80; //atoi(argv[2]);
    const char* request =  "GET /WackoWiki/KursOperacionnyeSistemy/PraktikumPosixThreads/PthreadTasks HTTP/1.1\r\n"
                           "Host: parallels.nsu.ru\r\n\r\n"; // argv[3];

    const int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    ASSERT(socket_fd >= 0);

    struct sockaddr_in server_addr;
    name2addr(host_name, host_port, &server_addr);
    ASSERT(connect(socket_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) == 0);
    write_(request, strlen(request), socket_fd);


    char* in_buffer = malloc(IN_BUFFER_SIZE);
    size_t received = 0;
    size_t written = 0;

    struct pollfd fds[3];

    size_t cnt_on_screen = 0;
    int is_end = 0;
    while(!is_end || written != received){
        fds[0].fd = 0;
        fds[0].events = POLLIN;
        fds[1].fd = socket_fd;
        fds[1].events = POLLIN;
        ASSERT(poll(fds, 2, -1) != -1);

        if (fds[1].revents & POLLIN){
            ssize_t cnt = read(socket_fd,in_buffer+received, IN_BUFFER_SIZE-received);
            if(cnt == 0){
                is_end = 1;
            }
            received += cnt;
        }

        if(fds[0].revents & POLLIN){
            char __b[1024];
            read(0, __b, 1024);
            cnt_on_screen = 0;
        }

        if (fds[0].revents & POLLIN || fds[1].revents & POLLIN){
            size_t to_write = MIN(received - written, MX_SCREEN_CHARS - cnt_on_screen);
            if(to_write > 0){
                write_(in_buffer + written, to_write, 1);
                written += to_write;
                cnt_on_screen += to_write;
            }
        }
    }

    close(socket_fd);
    free(in_buffer);

    return 0;
}
