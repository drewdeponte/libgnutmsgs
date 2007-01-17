#include "gnut_common.h"

int gnut_get_host_byte_order(void) {
    /* Here I use a union to create both a uint16_t integer as well as
     * an unsigned char array within the same memory area so that only
     * sizeof(uint16_t) is allocated and it can be referred to by either
     * data type. */
    union {
        uint16_t my_var;
        unsigned char my_data[sizeof(uint16_t)];
    } foo;

    // Set the short int to the big-endian version of some
    // destinguishable value. */
    foo.my_var = 0x0102;

    if ((foo.my_data[0] == 0x01) && (foo.my_data[1] == 0x02)) {
        /* this is big-endian */
        return 2;
    } else if ((foo.my_data[0] == 0x02) && (foo.my_data[1] == 0x01)) {
        /* this is little-endian */
        return 1;
    }

    /* Unknown byte order */
    return 0;
}

uint16_t gnut_liltobigs(uint16_t lilshort) {
    int size, i;
    unsigned char buff[sizeof(uint16_t)];

    size = sizeof(uint16_t);

    for (i = 0; i < size; i++) {
        buff[i] = ((unsigned char *)&lilshort)[(size - i - 1)];
    }

    return *(uint16_t *)buff;
}

uint32_t gnut_liltobigl(uint32_t lillong) {
    int size, i;
    unsigned char buff[sizeof(uint32_t)];

    size = sizeof(uint32_t);

    for (i = 0; i < size; i++) {
        buff[i] = ((unsigned char *)&lillong)[(size - i - 1)];
    }

    return *(uint32_t *)buff;
}

uint16_t gnut_bigtolils(uint16_t bigshort) {
    return gnut_liltobigs(bigshort);
}

uint32_t gnut_bigtolill(uint32_t biglong) {
    return gnut_liltobigl(biglong);
}
