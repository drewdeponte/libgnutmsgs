#include "gnut_query_msg.h"

int _gnut_parse_query_msg_payload(gnut_query_payload_t *pl,
    unsigned char *raw_pl, uint32_t raw_pl_len) {

    unsigned char *tmp_p;
    int search_str_len;

    tmp_p = raw_pl;

    pl->min_kb_speed = *((uint16_t *)tmp_p);
    tmp_p = tmp_p + sizeof(uint16_t);

    search_str_len = strlen((const char *)tmp_p);

    if (search_str_len == (raw_pl_len - sizeof(uint16_t) - 1)) {
        pl->search_str = malloc(search_str_len + 1);
        if (pl->search_str == NULL) {
            return -2;
        }
    } else if (search_str_len > (raw_pl_len - sizeof(uint16_t))) {
        /* SHIT SHIT SHIT, this is a problem, this means that we have no
         * null before the end of the message. */
        return -1;
    } else {
        pl->search_str = malloc(search_str_len + 1);
        if (pl->search_str == NULL) {
            return -3;
        }
    }

    memcpy((void *)pl->search_str, (const void *)tmp_p, search_str_len);
    pl->search_str[search_str_len] = '\0';

    pl->search_str_size = search_str_len + 1;

    return 0;
}

void _gnut_free_query_msg_payload(gnut_query_payload_t *pl) {
    if (pl->search_str != NULL)
        free(pl->search_str);
}

void _gnut_dump_query_msg_payload(const gnut_query_payload_t *pl) {
    printf("-- Min Kb Speed: %d.\n", pl->min_kb_speed);
    printf("-- Search Str: %s.\n", pl->search_str);
}
