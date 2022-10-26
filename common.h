#ifndef COMMON_H
#define COMMON_H

#define CEM(message, code) \
do{\
    if(code != 0){ \
        perror(message); \
        exit(1); \
    } \
}while(0)

#define CE(code) \
do{\
    if(code != 0){ \
        exit(1); \
    } \
}while(0)

#endif