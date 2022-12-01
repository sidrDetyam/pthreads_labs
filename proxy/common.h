//
// Created by argem on 30.11.2022.
//

#ifndef PTHREAD_COMMON_H
#define PTHREAD_COMMON_H

enum STATUS{
    ERROR = -1,
    SUCCESS = 0
};


#define ASSERT(cond__) \
do{\
    if(!(cond__)){ \
        perror("something goes wrong..."); \
        printf(#cond__);                   \
        printf("\n");  \
        exit(1); \
    } \
}while(0)

#define ASSERT_RETURN(cond__) \
do{\
    if(!(cond__)){ \
        return ERROR; \
    } \
}while(0)

#define ASSERT_RETURN2(cond__, ret__) \
do{\
    if(!(cond__)){ \
        return ret__; \
    } \
}while(0)

#define ASSERT_RETURN_C(cond__, on_fail__) \
do{\
    if(!(cond__)){ \
        on_fail__;    \
        return ERROR; \
    } \
}while(0)



#endif //PTHREAD_COMMON_H
