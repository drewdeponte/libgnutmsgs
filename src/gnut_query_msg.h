#ifndef GNUT_QUERY_MSG_H
#define GNUT_QUERY_MSG_H

#include "gnut_common.h"

typedef struct gnut_query_payload {
    uint16_t min_kb_speed;
    char *search_str;
    int search_str_size;
} gnut_query_payload_t;

int _gnut_parse_query_msg_payload(gnut_query_payload_t *pl,
    unsigned char *raw_pl, uint32_t raw_pl_len);

void _gnut_free_query_msg_payload(gnut_query_payload_t *pl);

void _gnut_dump_query_msg_payload(const gnut_query_payload_t *pl);

#endif
