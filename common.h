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
        perror("something goes wrong...");\
        exit(1); \
    } \
}while(0)

#define ASSERT(cond__) \
do{\
    if(!(cond__)){ \
        perror("something goes wrong...");\
        exit(1); \
    } \
}while(0)

#define MAX(a__, b__) ((a__) < (b__) ? (b__) : (a__))

#define MIN(a__, b__) ((a__) > (b__) ? (b__) : (a__))

#endif