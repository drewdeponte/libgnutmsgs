#ifndef GNUT_BYE_MSG_H
#define GNUT_BYE_MSG_H

#include <stdint.h>
#include <netinet/in.h>
#include "gnut_common.h"

typedef struct gnut_bye_payload {
    uint16_t code;
    char *desc_string;
    uint16_t desc_string_len;
} gnut_bye_payload_t;

int _gnut_parse_bye_msg_payload(gnut_bye_payload_t *pl,
    unsigned char *raw_pl, uint32_t raw_pl_len);

void _gnut_free_bye_msg_payload(gnut_bye_payload_t *pl);

void _gnut_dump_bye_msg_payload(const gnut_bye_payload_t *pl);

#endif
