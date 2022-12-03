//
// Created by argem on 30.11.2022.
//

#ifndef PTHREAD_COMMON_H
#define PTHREAD_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

enum STATUS{
    ERROR = -1,
    SUCCESS = 0
};


#define ASSERT(cond__) \
do{\
    if(!(cond__)){ \
        perror("something goes wrong..."); \
        fprintf(stderr, #cond__);                   \
        fprintf(stderr, "\n");  \
        exit(1); \
    } \
}while(0)

#define ASSERT_RETURN(cond__) \
do{\
    if(!(cond__)){            \
        fprintf(stderr, "something goes wrong... "); \
        fprintf(stderr, #cond__);                   \
        fprintf(stderr, "\n");        \
        return ERROR; \
    } \
}while(0)

#define ASSERT_RETURN2(cond__, ret__) \
do{\
    if(!(cond__)){                    \
        fprintf(stderr, "something goes wrong... "); \
        fprintf(stderr, #cond__);                   \
        fprintf(stderr, "\n");        \
        return ret__; \
    } \
}while(0)

#define ASSERT_RETURN_C(cond__, on_fail__) \
do{\
    if(!(cond__)){                         \
        fprintf(stderr, "something goes wrong... "); \
        fprintf(stderr, #cond__);                   \
        fprintf(stderr, "\n");        \
        on_fail__;    \
        return ERROR; \
    } \
}while(0)

#define ASSERT_RETURN2_C(cond__, on_fail__, ret__) \
do{\
    if(!(cond__)){                                 \
        fprintf(stderr, "something goes wrong... "); \
        fprintf(stderr, #cond__);                   \
        fprintf(stderr, "\n");        \
        on_fail__;    \
        return ret__; \
    } \
}while(0)

#endif //PTHREAD_COMMON_H
