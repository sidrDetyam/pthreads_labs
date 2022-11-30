//
// Created by argem on 30.11.2022.
//

#ifndef PTHREAD_HTTP_HEADER_PARSER_H
#define PTHREAD_HTTP_HEADER_PARSER_H

#include<stdlib.h>

enum REQUEST_TYPE{
    GET,
    UNKNOWN
};

struct HEADER{
    const char* type;
    size_t tlen;
    const char* value;
    size_t vlen;
};
typedef struct HEADER header_t;

#define ELEMENT_TYPE header_t
#include "CVector_def.h"


struct PARSER{
    int is_req_type_parsed;
    int req_type;
    char* uri;

    int is_parsed;
    size_t pos;
    const char* buff;
    vheader_t headers;
};
typedef struct PARSER parser_t;

void
parser_init(parser_t* parser, const char* buff);

void
parser_destroy(parser_t* parser);

enum PARSE_STATUS{
    END_OF_HEADER,
    NO_END_OF_LINE,
    PARSING_ERROR,
    PARSED,
    HAS_NEXT
};

int
parse_next(parser_t* parser);

#endif //PTHREAD_HTTP_HEADER_PARSER_H
