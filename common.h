#ifndef COMMON_H
#define COMMON_H

#define CHECK_ERROR(message, code) \
do{\
    if(code == -1){ \
        perror(message); \
        exit(1); \
    } \
}while(0)

#endif