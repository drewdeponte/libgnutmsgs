#include "gnut_query_hit_msg.h"

gnut_query_hit_result_t *create_empty_query_hit_result_set(void) {
    return NULL;
}

gnut_query_hit_result_t *append_query_hit_result_to_set(
    gnut_query_hit_result_t *list,
    uint32_t file_index, uint32_t file_size, const char *file_name,
    unsigned char *exten_block) {

    gnut_query_hit_result_t *new_result;
    gnut_query_hit_result_t *last_result;
    gnut_query_hit_result_t *cur_result;
    int file_name_str_len;

    cur_result = list;
    last_result = NULL;

    while (cur_result != NULL) {
        last_result = cur_result;
        cur_result = cur_result->next;
    }

    file_name_str_len = strlen(file_name);

    /* Allocate memory for new result in linked list. */
    new_result = (gnut_query_hit_result_t *)malloc(sizeof(gnut_query_hit_result_t));
    if (new_result == NULL) {
        return NULL;
    }

    /* Copy the passed values over to the newly allocated result in the
     * linked list. */
    new_result->file_index = file_index;
    new_result->file_size = file_size;
    new_result->file_name = malloc(file_name_str_len + 1);
    if (new_result->file_name == NULL) {
        free(new_result);
        return NULL;
    }
    strncpy(new_result->file_name, file_name, file_name_str_len);
    new_result->file_name[file_name_str_len] = 0x00;
    new_result->exten_block = (unsigned char *)malloc(strlen((char *)exten_block));
    if (new_result->exten_block == NULL) {
        free(new_result->file_name);
        free(new_result);
        return NULL;
    }
    memcpy((void *)new_result->exten_block, (const void *)exten_block,
        strlen((char *)exten_block));
    new_result->next = NULL;

    if (last_result == NULL) {
        return new_result;
    } else {
        last_result->next = new_result;
        return list;
    }
}

void _gnut_free_query_hit_msg_payload(gnut_query_hit_payload_t *pl) {
    gnut_query_hit_result_t *p_cur_res;
    gnut_query_hit_result_t *p_next_res;

    printf("Entered _gnut_free_query_hit_msg_payload().\n");

    /* Free all the previously alloced gnut_query_hit_results associated
     * with the given payload. */
    p_cur_res = pl->list;
    while (p_cur_res != NULL) {
        p_next_res = p_cur_res->next;
        if (p_cur_res->file_name != NULL) {
            free(p_cur_res->file_name);
            p_cur_res->file_name = NULL;
        }
        if (p_cur_res->exten_block != NULL) {
            free(p_cur_res->exten_block);
            p_cur_res->exten_block = NULL;
        }
        free(p_cur_res);
        p_cur_res = p_next_res;
    }

    pl->list = NULL;

    printf("Freed results list.\n");

    if (pl->extra_block.open_data != NULL) {
        free(pl->extra_block.open_data);
        pl->extra_block.open_data = NULL;
    }

    printf("Freed extra block.\n");

    if (pl->priv_data != NULL) {
        free(pl->priv_data);
        pl->priv_data = NULL;
    }
}

int _gnut_build_query_hit_msg_payload(gnut_query_hit_payload_t *pl,
    unsigned char *raw_pl) {

    unsigned char *p_raw_pl;
    gnut_query_hit_result_t *cur_result;

    p_raw_pl = raw_pl;
    
    /* Fill the number of hits into the raw payload */
    *((unsigned char *)p_raw_pl) = pl->num_hits;
    p_raw_pl = p_raw_pl + sizeof(unsigned char);

    /* Fill the port number in the raw payload */
    *((uint16_t *)p_raw_pl) = pl->port_num;
    p_raw_pl = p_raw_pl + sizeof(uint16_t);

    /* Fill the ip address in the raw payload */
    *((uint32_t *)p_raw_pl) = (uint32_t)pl->ip_addr.s_addr;
    p_raw_pl = p_raw_pl + sizeof(uint32_t);

    /* Fill the speed in the raw payload */
    *((uint32_t *)p_raw_pl) = pl->speed;
    p_raw_pl = p_raw_pl + sizeof(uint32_t);

    /* Iterate through result set. */
    cur_result = pl->list;
    while (cur_result != NULL) {
        /* Fill file index in the raw payload */
        *((uint32_t *)p_raw_pl) = cur_result->file_index;
        p_raw_pl = p_raw_pl + sizeof(uint32_t);

        /* Fill in the file size in the raw payload */
        *((uint32_t *)p_raw_pl) = cur_result->file_size;
        p_raw_pl = p_raw_pl + sizeof(uint32_t);

        /* Fill in the file name in the raw payload */
        memcpy((void *)p_raw_pl, (void *)cur_result->file_name,
            strlen(cur_result->file_name));
        p_raw_pl = p_raw_pl + strlen(cur_result->file_name);
        *((char *)p_raw_pl) = 0x00;
        p_raw_pl = p_raw_pl + 1;

        /* Fill in the extension block in the raw payload */
        memcpy((void *)p_raw_pl, (void *)cur_result->exten_block,
            strlen((const char *)cur_result->exten_block));
        p_raw_pl = p_raw_pl + strlen((const char *)cur_result->exten_block);
        *((unsigned char *)p_raw_pl) = 0x00;
        p_raw_pl = p_raw_pl + 1;

        cur_result = cur_result->next;
    }

    /* Fill in the vendor code in the raw payload */
    memcpy((void *)p_raw_pl, (void *)pl->extra_block.ven_code, 4);
    p_raw_pl = p_raw_pl + 4;

    /* Fill in the open data size in the raw payload */
    *((unsigned char *)p_raw_pl) = pl->extra_block.open_data_size;
    p_raw_pl = p_raw_pl + 1;

    /* Fill in the open data in the raw payload */
    memcpy((void *)p_raw_pl, (void *)pl->extra_block.open_data,
        pl->extra_block.open_data_size);
    p_raw_pl = p_raw_pl + pl->extra_block.open_data_size;

    /* Fill in the private data in the raw payload */
    memcpy((void *)p_raw_pl, (void *)pl->priv_data, pl->priv_data_len);
    p_raw_pl = p_raw_pl + pl->priv_data_len;

    /* Fill in the servent id in the raw payload */
    memcpy((void *)p_raw_pl, (void *)pl->servent_id, GNUT_SERVENT_ID_LEN);
    p_raw_pl = p_raw_pl + GNUT_SERVENT_ID_LEN;

    return 0;
}

int _gnut_parse_query_hit_msg_payload(gnut_query_hit_payload_t *pl,
    unsigned char *raw_pl, uint32_t raw_pl_len) {

    unsigned char *tmp_p;
    uint32_t tmp_size;
    int i;
    uint32_t file_index;
    uint32_t file_size;
    char file_name[256];
    unsigned char exten_block[256];
    gnut_query_hit_result_t *res_set;
    
    res_set = NULL;
    tmp_p = raw_pl;
    tmp_size = 0;

    pl->num_hits = *((unsigned char *)tmp_p);
    tmp_p += sizeof(unsigned char);
    tmp_size += sizeof(unsigned char);

    pl->port_num = *((uint16_t *)tmp_p);
    tmp_p += sizeof(uint16_t);
    tmp_size += sizeof(unsigned char);

    pl->ip_addr.s_addr = *((uint32_t *)tmp_p);
    tmp_p += sizeof(uint32_t);
    tmp_size += sizeof(uint32_t);

    pl->speed = *((uint32_t *)tmp_p);
    tmp_p += sizeof(uint32_t);
    tmp_size += sizeof(uint32_t);

    for(i = 0; i < pl->num_hits; i++) {
        file_index = *((uint32_t *)tmp_p);
        tmp_p += sizeof(uint32_t);
        tmp_size += sizeof(uint32_t);

        file_size = *((uint32_t *)tmp_p);
        tmp_p += sizeof(uint32_t);
        tmp_size += sizeof(uint32_t);

        memset((void *)file_name, '\0', 256);
        strncpy(file_name, ((char *)tmp_p), 255);
        file_name[255] = '\0';
        tmp_p += strlen(file_name);
        tmp_size += strlen(file_name);
        
        memset((void *)exten_block, '\0', 256);
        strncpy((char *)exten_block, ((char *)tmp_p), 255);
        exten_block[255] = '\0';
        tmp_p += strlen((char *)exten_block);
        tmp_size += strlen((char *)exten_block);

        res_set = append_query_hit_result_to_set(res_set, file_index,
            file_size, file_name, exten_block);
    }

    pl->list = res_set;

    // At this point I want to parse the extra block.
    memcpy(pl->extra_block.ven_code, (unsigned char *)tmp_p, 4);
    tmp_p += 4;
    tmp_size += 4;

    pl->extra_block.open_data_size = *((unsigned char *)tmp_p);
    tmp_p += sizeof(unsigned char);
    tmp_size += sizeof(unsigned char);

    pl->extra_block.open_data = malloc(pl->extra_block.open_data_size);
    if (pl->extra_block.open_data == NULL) {
        return -1;
    }

    memcpy(pl->extra_block.open_data, (unsigned char *)tmp_p,
        pl->extra_block.open_data_size);
    tmp_p += pl->extra_block.open_data_size;
    tmp_size += pl->extra_block.open_data_size;

    pl->priv_data_len = raw_pl_len - tmp_size - GNUT_SERVENT_ID_LEN;
    pl->priv_data = malloc(pl->priv_data_len);
    if (pl->priv_data == NULL) {
        return -2;
    }

    memcpy((void *)pl->priv_data, tmp_p, pl->priv_data_len);
    tmp_p += pl->priv_data_len;

    memcpy((void *)pl->servent_id, tmp_p, GNUT_SERVENT_ID_LEN);
    tmp_p += GNUT_SERVENT_ID_LEN;

    return 0;
}

int _gnut_calc_query_hit_msg_pl_len(gnut_query_hit_payload_t *pl) {
    int pl_len;
    gnut_query_hit_result_t *cur_result;

    /* Account for the number of hits, port, ip address, and speed */
    pl_len = 1 + sizeof(uint16_t) + sizeof(uint32_t) + sizeof(uint32_t);

    cur_result = pl->list;
    while (cur_result != NULL) {
        /* Account for the file index, file size, file name, and
         * terminating null byte for the file name */
        pl_len = pl_len + sizeof(uint32_t) + sizeof(uint32_t) + \
            strlen(cur_result->file_name) + 1;

        /* Account for the exten block and it's terminating null byte */
        if (cur_result->exten_block != NULL) {
            pl_len = pl_len + strlen((const char *)cur_result->exten_block);
        }
        pl_len = pl_len + 1;

        cur_result = cur_result->next;
    }

    /* Account for the vendor code, open data size, and open data of the
     * extra block */
    pl_len = pl_len + 4 + sizeof(unsigned char) + \
        pl->extra_block.open_data_size;

    /* Account for the private data */
    pl_len = pl_len + pl->priv_data_len;

    /* Account for the servent id */
    pl_len = pl_len + GNUT_SERVENT_ID_LEN;

    return pl_len;
}
