#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

int main(int argc,char *argv[]){

    struct hostent *server;
    struct sockaddr_in serv_addr;

    int sockfd = socket(AF_INET, SOCK_STREAM, PF_INET);
    if (sockfd < 0) {
        error("ERROR opening socket");
        exit(0);
    }

    char* host = "parallels.nsu.ru";
    char* message = "GET /WackoWiki/KursOperacionnyeSistemy/PraktikumPosixThreads/PthreadTasks HTTP/1.1\r\nHost: parallels.nsu.ru\r\n\r\n";

    server = gethostbyname(host);
    if (server == NULL) {
        error("ERROR, no such host");
        exit(0);
    }

    memset(&serv_addr,0,sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(80);
    memcpy(&serv_addr.sin_addr.s_addr,server->h_addr,server->h_length);

    if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0) {
        error("ERROR connecting");
        exit(0);
    }

    int total = strlen(message);
    int sent = 0;
    do {
        int bytes = write(sockfd,message+sent,total-sent);
        if (bytes < 0)
            error("ERROR writing message to socket");
        if (bytes == 0)
            break;
        sent+=bytes;
    } while (sent < total);

    /* receive the response */
    total = 1000000;
    char *response = malloc(total);
    memset(response,0, total);
    int received = 0;
    do {
        int bytes = read(sockfd,response+received,total-received);
        if (bytes < 0) {
            //error("ERROR reading response from socket");
            break;
        }
        if (bytes == 0)
            break;
        received+=bytes;
        //break;
    } while (1);

    /*
     * if the number of received bytes is the total size of the
     * array then we have run out of space to store the response
     * and it hasn't all arrived yet - so that's a bad thing
     */

    /* close the socket */
    close(sockfd);

    printf("Response:\n%s\n",response);

    //free(message);
    return 0;
}
