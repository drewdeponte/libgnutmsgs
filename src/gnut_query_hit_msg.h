#ifndef GNUT_QUERY_HIT_MSG_H
#define GNUT_QUERY_HIT_MSG_H

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/in.h>
#include "gnut_common.h"

struct gnut_query_hit_extra_block {
    unsigned char ven_code[4];      /* Four byte Vendor Code */
    unsigned char open_data_size;   /* Open Data size in bytes */
    unsigned char *open_data;       /* The Open Data */
};

typedef struct gnut_query_hit_result {
    uint32_t file_index;    /* Num assigned to file by responding node */
    uint32_t file_size;     /* Size (in bytes) of the associated file */
    char *file_name;        /* C-string (null terminated) */
    unsigned char *exten_block;         /* Just a null byte for now */
    struct gnut_query_hit_result *next; /* Next query hit result */
} gnut_query_hit_result_t;

typedef struct gnut_query_hit_payload {
    unsigned char num_hits; /* Num of hits in result set */
    uint16_t port_num;      /* Accepting file req port on respond node */
    struct in_addr ip_addr; /* IP addr of responding host */
    uint32_t speed;         /* Speed of responding node in kb/second */
    gnut_query_hit_result_t *list;  /* Results liked list head */
    struct gnut_query_hit_extra_block extra_block; /* Extra block */
    unsigned char *priv_data;       /* Private data */
    uint32_t priv_data_len;         /* Private data length */
    unsigned char servent_id[GNUT_SERVENT_ID_LEN];  /* Servent id */
} gnut_query_hit_payload_t;

gnut_query_hit_result_t *create_empty_query_hit_result_set(void);

/**
 * Append Query Hit Result to Query Hit Result Set.
 *
 * Append Query Hit Result to the specified Query Hit Result Set.
 * \param list Pointer to linked list representing query hit result set.
 * \param file_index File index associated with the file by responder.
 * \param file_size Size of file in bytes.
 * \param file_name C-String of the name of the file.
 * \return Pointer to head of linked list represting query hit res set.
 * \retval NULL Error, failed to append query hit result to set.
 */
gnut_query_hit_result_t *append_query_hit_result_to_set(
    gnut_query_hit_result_t *list, uint32_t file_index, uint32_t file_size,
    const char *file_name, unsigned char *exten_block);

void _gnut_free_query_hit_msg_payload(gnut_query_hit_payload_t *pl);

int _gnut_build_query_hit_msg_payload(gnut_query_hit_payload_t *pl,
    unsigned char *raw_pl);

int _gnut_calc_query_hit_msg_pl_len(gnut_query_hit_payload_t *pl);

#endif
