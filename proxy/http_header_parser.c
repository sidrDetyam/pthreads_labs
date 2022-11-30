//
// Created by argem on 01.12.2022.
//

#include "http_header_parser.h"

#include <string.h>

#define ELEMENT_TYPE header_t
#include "CVector_impl.h"

void
parser_init(parser_t* parser, const char* buff){
    parser->buff = buff;
    parser->pos = 0;
    parser->is_req_type_parsed = 0;
    parser->is_parsed = 0;
    vheader_t_init(&parser->headers);
}

void
parser_destroy(parser_t* parser){

}

static const char*
curr(parser_t* parser){
    return parser->buff + parser->pos;
}

static void
set_req_type(parser_t* parser, const char* type, size_t type_len){
    if(strncmp("GET", type, type_len) == 0){
        parser->req_type = GET;
        return;
    }
    parser->req_type = UNKNOWN;
}

int
parse_next(parser_t* parser){
    if(parser->is_parsed){
        return PARSED;
    }
    const char* end = strstr(curr(parser), "\r\n");
    if(end == NULL){
        return NO_END_OF_LINE;
    }

    if(!parser->is_req_type_parsed){
        const char* ws = strstr(curr(parser), " ");
        if(ws == NULL){
            return PARSING_ERROR;
        }
        set_req_type(parser, curr(parser), ws - curr(parser));
        const char* ws2 = strstr(ws+1, " ");
        if(ws2 == NULL){
            return PARSING_ERROR;
        }
        parser->uri = malloc(ws2 - ws);
        parser->uri[ws2-ws-1] = '\0';
        memcpy(parser->uri, ws + 1, ws2 - ws - 1);

        parser->pos += end - curr(parser) + 2;
        parser->is_req_type_parsed = 1;
        return HAS_NEXT;
    }

    if(end == curr(parser)){
        parser->is_parsed = 1;
        return PARSED;
    }

    const char* sep = strstr(curr(parser), ": ");
    if(sep == NULL){
        return PARSING_ERROR;
    }

    header_t header;
    header.type = curr(parser);
    header.tlen = sep - curr(parser);
    header.value = sep+2;
    header.vlen = end - (sep+2);
    vheader_t_push_back(&parser->headers, &header);
    parser->pos += end - curr(parser) + 2;

    return HAS_NEXT;
}