#ifndef GNUT_COMMON_H
#define GNUT_COMMON_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GNUT_MSG_HEADER_SIZE 23 /* Total Size of Msg Headers in Bytes */
#define GNUT_MSG_ID_LEN 16      /* Total Size of Msg IDs in Bytes */
#define GNUT_PORT 6346
#define BUFSIZE 2048
#define GNUT_SERVENT_ID_LEN 16

/**
 * Get the byte order of the host.
 *
 * Get the byte order of the host machine.
 * \return An integer representing the host byte order.
 * \retval 1 The host byte order is little-endian.
 * \retval 2 The host byte order is big-endian.
 * \retval 0 The host byte order is unknown.
 */
int gnut_get_host_byte_order(void);

/**
 * Convert little-endian short to a big-endian short.
 *
 * The gnut_liltobigs function converts a little-endian uint16_t to a
 * big-endian uint16_t.
 * \param lilshort The lil-end uint16_t to convert to big-end uint16_t.
 * \return The big-endian version of the given little-endian uint16_t.
 */
uint16_t gnut_liltobigs(uint16_t lilshort);

/**
 * Convert little-endian long to big-endian long.
 *
 * The gnut_liltobigl function converts a little-endian uint32_t to a
 * big-endian uint32_t.
 * \param lillong The lil-end uint32_t to convert to big-end uint32_t.
 * \return The big-endian version of given little-endian uint32_t.
 */
uint32_t gnut_liltobigl(uint32_t lillong);

/**
 * Convert big-endian short to a little-endian short.
 *
 * The gnut_bigtolils function converts a big-endian uint16_t to a
 * little-endian uint16_t.
 * \param bigshort The big-end uint16_t to convert to lil-end uint16_t.
 * \return The little-endian version of given big-endian uint16_t.
 */
uint16_t gnut_bigtolils(uint16_t bigshort);

/**
 * Convert big-endian long to a little-endian long.
 *
 * The gnut_bigtolill function converts a big-endian uint32_t to a
 * little-endian uint32_t.
 * \param biglong The big-end uint32_t to convert to lil-end uint32_t.
 * \return The little-endian version of given big-endian uint32_t.
 */
uint32_t gnut_bigtolill(uint32_t biglong);

#endif
