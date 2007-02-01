#include "gnut_query_msg.h"

int _gnut_parse_query_msg_payload(gnut_query_payload_t *pl,
    unsigned char *raw_pl, uint32_t raw_pl_len) {

    unsigned char *tmp_p;
    int search_str_len;     /* size in bytes not including NULL byte */
    int search_str_size;    /* size in bytes including NULL byte */

    tmp_p = raw_pl;

    pl->min_kb_speed = *((uint16_t *)tmp_p);
    tmp_p = tmp_p + sizeof(uint16_t);

    search_str_len = strlen((const char *)tmp_p);
    search_str_size = search_str_len + 1;

    if (search_str_size == (raw_pl_len - sizeof(uint16_t))) {
        /* I know that there is no other data after the search str */
        pl->search_str = malloc(search_str_size);
        if (pl->search_str == NULL) {
            return -2;
        }
        
        memcpy((void *)pl->search_str, (const void *)tmp_p,
            search_str_len);
        pl->search_str[search_str_len] = '\0';
        pl->search_str_size = search_str_size;
        tmp_p = tmp_p + search_str_size;

        pl->rest = NULL;
        pl->rest_size = 0;
    } else if (search_str_size < (raw_pl_len - sizeof(uint16_t))) {
        /* I know that there is more data after the search string */
        pl->search_str = malloc(search_str_size);
        if (pl->search_str == NULL) {
            return -3;
        }
    
        memcpy((void *)pl->search_str, (const void *)tmp_p,
            search_str_len);
        pl->search_str[search_str_len] = '\0';
        pl->search_str_size = search_str_size;
        tmp_p = tmp_p + search_str_size;

        pl->rest_size = (raw_pl_len - sizeof(uint16_t) - search_str_size);
        pl->rest = malloc(pl->rest_size);
        if (pl->rest == NULL) {
            return -4;
        }

        memcpy((void *)pl->rest, (const void *)tmp_p,
            pl->rest_size);
        tmp_p = tmp_p + pl->rest_size;

    } else {
        /* This is an error search_str_size is larger than the space
         * existing after the raw_pl_len - sizeof min_kb_speed */
        return -1;
    }

    return 0;
}

int _gnut_build_query_msg_payload(gnut_query_payload_t *pl,
    unsigned char *raw_pl) {

    unsigned char *tmp_p;

    tmp_p = raw_pl;

    *((uint16_t *)tmp_p) = pl->min_kb_speed;
    tmp_p = tmp_p + sizeof(uint16_t);

    //printf("_gnut_build_query_msg_payload() seach_str_size = %d.\n",
    //    pl->search_str_size);

    memcpy(tmp_p, pl->search_str, (pl->search_str_size - 1));
    tmp_p[pl->search_str_size - 1] = '\0';

    tmp_p = tmp_p + pl->search_str_size;

    memcpy(tmp_p, pl->rest, pl->rest_size);
    tmp_p = tmp_p + pl->rest_size;

    return 0;
}

void _gnut_free_query_msg_payload(gnut_query_payload_t *pl) {
    if (pl->search_str != NULL)
        free(pl->search_str);
    if (pl->rest != NULL)
        free(pl->rest);
}

void _gnut_dump_query_msg_payload(const gnut_query_payload_t *pl) {
    printf("-- Min Kb Speed: %d.\n", pl->min_kb_speed);
    printf("-- Search Str: %s.\n", pl->search_str);
}

int _gnut_calc_query_msg_pl_len(gnut_query_payload_t *pl) {
    return (sizeof(uint16_t) + pl->search_str_size + pl->rest_size);
}
