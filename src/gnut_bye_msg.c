#include "gnut_bye_msg.h"

int _gnut_parse_bye_msg_payload(gnut_bye_payload_t *pl,
    unsigned char *raw_pl, uint32_t raw_pl_len) {

    unsigned char *tmp_p;
    int desc_string_len;

    tmp_p = raw_pl;

    pl->code = *((uint16_t *)tmp_p);
    tmp_p = tmp_p + sizeof(uint16_t);

    desc_string_len = strlen((const char *)tmp_p);

    if (desc_string_len == (raw_pl_len - sizeof(uint16_t) - 1)) {
        pl->desc_string = malloc(desc_string_len + 1);
        if (pl->desc_string == NULL) {
            return -2;
        }
    } else if (desc_string_len > (raw_pl_len - sizeof(uint16_t))) {
        /* This is a problem, this means that we have no null before the
         * end of the message. */
        return -1;
    } else {
        pl->desc_string = malloc(desc_string_len + 1);
        if (pl->desc_string == NULL) {
            return -3;
        }
    }

    memcpy((void *)pl->desc_string, (const void *)tmp_p, desc_string_len);
    pl->desc_string[desc_string_len] = '\0';

    pl->desc_string_len = desc_string_len + 1;

    return 0;
}

void _gnut_free_bye_msg_payload(gnut_bye_payload_t *pl) {
    if (pl->desc_string != NULL)
        free(pl->desc_string);
}

void _gnut_dump_bye_msg_payload(const gnut_bye_payload_t *pl) {
    printf("-- Code: %d.\n", pl->code);
    printf("-- Description String:\n%s", pl->desc_string);
}
