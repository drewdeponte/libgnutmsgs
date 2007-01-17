#include "gnut_pong_msg.h"

int _gnut_build_pong_msg_payload(gnut_pong_payload_t *pl,
    unsigned char *raw_pl) {

    unsigned char *tmp_p;

    tmp_p = raw_pl;

    *((uint16_t *)tmp_p) = pl->port_num;
    tmp_p = tmp_p + sizeof(uint16_t);

    *((uint32_t *)tmp_p) = (uint32_t)pl->ip_addr.s_addr;
    tmp_p = tmp_p + sizeof(uint32_t);

    *((uint32_t *)tmp_p) = (uint32_t)pl->num_shared_files;
    tmp_p = tmp_p + sizeof(uint32_t);

    *((uint32_t *)tmp_p) = (uint32_t)pl->kb_shared;

    return 0;
}

int _gnut_calc_pong_msg_pl_len(gnut_pong_payload_t *pl) {
    return (sizeof(uint16_t) + sizeof(uint32_t) + sizeof(uint32_t) + \
        sizeof(uint32_t));
}

void _gnut_free_pong_msg_payload(gnut_pong_payload_t *pl) {
    return;
}
