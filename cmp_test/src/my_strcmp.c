//
// Created by argem on 02.11.2022.
//

#include "my_strcmp.h"

enum COMP_RES
str_comp(const char *s1_, const char *s2_) {
    const unsigned char* s1 = (const unsigned char*) s1_;
    const unsigned char* s2 = (const unsigned char*) s2_;

    size_t i;
    for (i = 0; s1[i] != '\0' || s2[i] != '\0'; i++) {
        if (s1[i] > s2[i]) {
            return GREATER;
        }
        if (s2[i] > s1[i]) {
            return LESS;
        }
    }

    if (s1[i] == '\0' && s2[i] == '\0') {
        return EQ;
    }

    if (s1[i] != '\0') {
        return GREATER;
    }

    return LESS;
}