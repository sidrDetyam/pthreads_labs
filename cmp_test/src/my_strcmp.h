//
// Created by argem on 02.11.2022.
//

#ifndef PTHREAD_MY_STRCMP_H
#define PTHREAD_MY_STRCMP_H

#include <stdlib.h>

enum COMP_RES {
    LESS = -1,
    EQ,
    GREATER
};


//if data change - UB
//s1 and s2 not null

enum COMP_RES
str_comp(const char *s1, const char *s2);

#endif //PTHREAD_MY_STRCMP_H
