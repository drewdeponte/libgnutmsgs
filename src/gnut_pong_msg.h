#ifndef GNUT_PONG_MSG_H
#define GNUT_PONG_MSG_H

#include <stdint.h>
#include <netinet/in.h>

typedef struct gnut_pong_payload {
    uint16_t port_num;
    struct in_addr ip_addr;
    uint32_t num_shared_files;
    uint32_t kb_shared;
} gnut_pong_payload_t;

int _gnut_build_pong_msg_payload(gnut_pong_payload_t *pl,
    unsigned char *raw_pl);

int _gnut_calc_pong_msg_pl_len(gnut_pong_payload_t *pl);

void _gnut_free_pong_msg_payload(gnut_pong_payload_t *pl);

#endif
