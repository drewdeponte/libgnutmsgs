/*
 * Copyright 2006 Andrew De Ponte, Josh Abbott
 *
 * This file is part of lib_gnut_msgs.
 *
 * lib_gnut_msgs is the intellectual property of Andrew De Ponte and
 * Josh Abbott; any distribution and/or modification and/or
 * reproductions of any portion of lib_gnut_msgs MUST be approved by
 * both Andrew De Ponte and Josh Abbott.
 *
 * lib_gnut_msgs is distributed WITHOUT ANY WARRANTY; without even the
 * implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.
 */

#include "gnut_msgs.h"

int gnut_build_msg_id(gnut_msg_hdr_t *p_header) {
    int i;
    
    srand((unsigned int)time(NULL));

    for (i = 0; i < GNUT_MSG_ID_LEN; i++) {
        p_header->message_id[i] = (char)(rand() % 255);
    }
    p_header->message_id[8] = 0xff;
    p_header->message_id[15] = 0x00;

    return 0;
}

int gnut_build_msg_hdr(gnut_msg_hdr_t *p_header, unsigned char type,
    uint32_t pl_len) {

    int r;

    r = gnut_build_msg_id(p_header);
    if (r != 0) {
        return -1;
    }

    p_header->type = type;
    p_header->ttl = 0x07;
    p_header->hops = 0x00;
    p_header->pl_len = pl_len;

    return 0;
}

int gnut_build_msg_hdr_given_msg_id(gnut_msg_hdr_t *p_header,
    const unsigned char *message_id, unsigned char type, uint32_t pl_len) {

    memcpy((void *)p_header->message_id, (const void *)message_id,
        GNUT_MSG_ID_LEN);
    
    p_header->type = type;
    p_header->ttl = 0x07;
    p_header->hops = 0x00;
    p_header->pl_len = pl_len;

    return 0;
}

int gnut_build_ping_msg(gnut_msg_t *p_msg) {
    int r;

    r = gnut_build_msg_hdr(&p_msg->header, 0x00, 0);
    if (r != 0) {
        return -1;
    }

    return 0;
}


int gnut_build_pong_msg(gnut_msg_t *p_msg, const unsigned char *msg_id,
    const char *ip_addr, uint16_t port_num, uint32_t num_shared_files,
    uint32_t kb_shared) {

    int r;

    r = gnut_build_msg_hdr_given_msg_id(&p_msg->header, msg_id, 0x01, 112);
    if (r != 0) {
        return -1;
    }

    if (inet_pton(AF_INET, ip_addr,
        (void *)&p_msg->payload.pong.ip_addr) <= 0) {
        return -2;
    }
    p_msg->payload.pong.port_num = port_num;
    p_msg->payload.pong.num_shared_files = num_shared_files;
    p_msg->payload.pong.kb_shared = kb_shared;

    return 0;
}

int gnut_build_query_hit_msg(gnut_msg_t *p_msg, const unsigned char *msg_id,
    unsigned char num_hits, uint16_t port_num, const char *ip_addr,
    uint32_t speed, gnut_query_hit_result_t *list,
    struct gnut_query_hit_extra_block *extra_block,
    unsigned char *priv_data, uint32_t priv_data_len,
    unsigned char *servent_id) {

    int r;
    /*
    unsigned char tmp[] = {0x54, 0x84, 0x96, 0x0E, 0x06, 0xc8, 0xa9, 0xf5, 0x4a, 0x0d, 0x42, 0xfc, 0xd4, 0x34, 0xd9, 0x00};
    unsigned char hitxtra[] = {0x75, 0x72, 0x6E, 0x3A, 0x73, 0x68, 0x61, 0x31, 0x3a, 0x32, 0x36, 0x52, 0x4f, 0x32, 0x50, 0x58, 0x34, 0x46, 0x43, 0x4b, 0x59, 0x43, 0x37, 0x43, 0x34, 0x45, 0x5a, 0x55, 0x33, 0x52, 0x33, 0x47, 0x4b, 0x51, 0x47, 0x50, 0x41, 0x54, 0x36, 0x55, 0x51, 0x1c, 0xc3, 0x82, 0x43, 0x54, 0x44, 0xeb, 0x9f, 0x56, 0x42};
    // 51 bytes
    unsigned char qhxtra[] = {0x4c, 0x49, 0x4d, 0x45, 0x04, 0x3c, 0x31, 0x01, 0x00, 0x01, 0xc3, 0x82, 0x42, 0x48, 0x40, 0x00};
    // 16 bytes
    */

    r = gnut_build_msg_hdr_given_msg_id(&p_msg->header, msg_id, 0x81, 0);
    if (r != 0) {
        return -1;
    }

    if (inet_pton(AF_INET, ip_addr,
        (void *)&p_msg->payload.query_hit.ip_addr) <= 0) {
        return -2;
    }
    p_msg->payload.query_hit.port_num = port_num;
    p_msg->payload.query_hit.num_hits = num_hits;
    p_msg->payload.query_hit.speed = speed;
    p_msg->payload.query_hit.list = list;
    memcpy((void *)p_msg->payload.query_hit.extra_block.ven_code,
        extra_block->ven_code, 4);
    p_msg->payload.query_hit.extra_block.open_data = extra_block->open_data;
    p_msg->payload.query_hit.extra_block.open_data_size = extra_block->open_data_size;
    p_msg->payload.query_hit.priv_data = priv_data;
    p_msg->payload.query_hit.priv_data_len = priv_data_len;

    memcpy((void *)p_msg->payload.query_hit.servent_id,
        (void *)servent_id, GNUT_SERVENT_ID_LEN);

    return 0;
}

int gnut_send_msg(gnut_msg_t *p_msg, int sd) {
    char *wire_buf;
    char *tmp_p;
    int wire_buf_size;
    ssize_t bytes_written;
    ssize_t tot_bytes_written;
    int is_big_end; /* flag representing if host is big-endian */
    int r;

    is_big_end = gnut_get_host_byte_order();
    if (is_big_end == 0) {
        return -5;
    }
    is_big_end = is_big_end - 1;
  
    r = _gnut_update_msg_pl_len(p_msg);
    if (r != 0) {
        return -6;
    }

    printf("pl_len = %d.\n", p_msg->header.pl_len);

    wire_buf_size = (GNUT_MSG_HEADER_SIZE + p_msg->header.pl_len);

    wire_buf = (char *)malloc((size_t)wire_buf_size);
    if (wire_buf == NULL) {
        return -1;
    }

    tmp_p = wire_buf;

    /* Copy the header into place in the wire buffer */
    memcpy((void *)tmp_p, (void *)p_msg->header.message_id, 16);
    tmp_p = tmp_p + 16;
    memcpy((void *)tmp_p, (void *)&p_msg->header.type, 1);
    tmp_p = tmp_p + 1;
    memcpy((void *)tmp_p, (void *)&p_msg->header.ttl, 1);
    tmp_p = tmp_p + 1;
    memcpy((void *)tmp_p, (void *)&p_msg->header.hops, 1);
    tmp_p = tmp_p + 1;
    if (is_big_end)
        *(uint32_t *)tmp_p = gnut_bigtolill(p_msg->header.pl_len);
    else
        *(uint32_t *)tmp_p = p_msg->header.pl_len;
    tmp_p = tmp_p + sizeof(uint32_t);

    /* Build and copy the payload into place in the wire buffer */
    r = _gnut_build_msg_payload(p_msg, (unsigned char *)tmp_p,
        p_msg->header.pl_len);
    if (r != 0) {
        free((void *)wire_buf);
        return -7;
    }

    /* Write the message */
    tot_bytes_written = 0;
    while (tot_bytes_written < wire_buf_size) {
        bytes_written = write(sd, (const void *)wire_buf,
            (size_t)wire_buf_size);
        if (bytes_written == -1) {
            perror("gnut_msgs: gnut_send_msg: write");
            free((void *)wire_buf);
            return -2;
        }
        
        tot_bytes_written = tot_bytes_written + bytes_written;
    }

    free((void *)wire_buf);

    return 0;
}

int gnut_recv_msg(gnut_msg_t *p_msg, int sd) {
    unsigned char tmp_hdr[GNUT_MSG_HEADER_SIZE];
    unsigned char *tmp_p;
    unsigned char *raw_payload;
    ssize_t tot_bytes_read;
    ssize_t bytes_read;
    int is_big_end; /* flag representing if host is big-endian */
    int r;

    is_big_end = gnut_get_host_byte_order();
    if (is_big_end == 0) {
        return -1;
    }
    is_big_end = is_big_end - 1;

    tot_bytes_read = 0;

    /* Read in the bytes that should compose the message header */
    while (tot_bytes_read < GNUT_MSG_HEADER_SIZE) {
        bytes_read = read(sd, (void *)(tmp_hdr + tot_bytes_read),
            (size_t)(GNUT_MSG_HEADER_SIZE - tot_bytes_read));
        if (bytes_read == 0) {
            /* Socket was closed on opposite end. */
            return -5;
        } else if (bytes_read == -1) {
            /* error */
            return -2;
        }
        tot_bytes_read = tot_bytes_read + bytes_read;
    }

    tmp_p = tmp_hdr;
    memcpy((void *)p_msg->header.message_id, (const void *)tmp_p,
        GNUT_MSG_ID_LEN);
    tmp_p = tmp_p + GNUT_MSG_ID_LEN;
    p_msg->header.type = *(tmp_p);
    tmp_p = tmp_p + 1;
    p_msg->header.ttl = *(tmp_p);
    tmp_p = tmp_p + 1;
    p_msg->header.hops = *(tmp_p);
    tmp_p = tmp_p + 1;
    if (is_big_end) {
        p_msg->header.pl_len = abs(gnut_liltobigl(*((uint32_t *)tmp_p)));
    } else {
        p_msg->header.pl_len = abs(*((uint32_t *)tmp_p));
    }
    tmp_p = tmp_p + sizeof(uint32_t);

    /* Use the payload length information from the previously obtained
     * message header to dynamically allocate a payload of the proper
     * size for the message and fill it.*/
    raw_payload = (unsigned char *)malloc((size_t)p_msg->header.pl_len);
    if (raw_payload == NULL)
        return -3;

    tot_bytes_read = 0;

    while (tot_bytes_read < p_msg->header.pl_len) {
        bytes_read = read(sd, (void *)(raw_payload + tot_bytes_read),
            (size_t)(p_msg->header.pl_len - tot_bytes_read));
        if (bytes_read == 0) {
            /* Socket was closed on opposite end. */
            free((void *)raw_payload);
            return -6;
        } else if (bytes_read == -1) {
            /* error */
            free((void *)raw_payload);
            return -4;
        }
        tot_bytes_read = tot_bytes_read + bytes_read;
    }
    
    r = _gnut_parse_msg_payload(p_msg, raw_payload, p_msg->header.pl_len);
    if (r != 0) {
        return -7;
    }

    return 0;
}

void gnut_free_msg(gnut_msg_t *p_msg) {
    unsigned char type;

    type = p_msg->header.type;

    if (type == 0x00) {

    } else if (type == 0x01) {
        _gnut_free_pong_msg_payload(&p_msg->payload.pong);
    } else if (type == 0x02) {
        _gnut_free_bye_msg_payload(&p_msg->payload.bye);
    } else if (type == 0x40) {

    } else if (type == 0x80) {
        _gnut_free_query_msg_payload(&p_msg->payload.query);
    } else if (type == 0x81) {
        _gnut_free_query_hit_msg_payload(&p_msg->payload.query_hit);
    }
}

int _gnut_parse_msg_payload(gnut_msg_t *msg, unsigned char *payload,
    uint32_t pl_len) {

    unsigned char type;

    type = msg->header.type;

    if(type == 0x00) {
        /* Ping, No Payload, nothing to parse */
    } else if(type == 0x01) {
        /* Pong, is payload */
    } else if(type == 0x02) {
        /* Bye, is payload */
        if (_gnut_parse_bye_msg_payload(&msg->payload.bye, payload,
            pl_len) != 0) {
            return -2;
        }
    } else if(type == 0x40) {
        /* Push, is payload */
    } else if(type == 0x80) {
        /* Query, is payload */
        if (_gnut_parse_query_msg_payload(&msg->payload.query, payload,
            pl_len) != 0) {
            return -1;
        }
    } else if(type == 0x81) {
        /* Query Hit, is payload */
    }

    return 0;
}

int _gnut_update_msg_pl_len(gnut_msg_t *msg) {
    unsigned char type;
    uint32_t pl_len;

    type = msg->header.type;

    pl_len = 0;

    if (type == 0x01) {
        pl_len = _gnut_calc_pong_msg_pl_len(&msg->payload.pong);
    } else if (type == 0x80) {
        pl_len = _gnut_calc_query_msg_pl_len(&msg->payload.query);
    } else if (type == 0x81) {
        pl_len = _gnut_calc_query_hit_msg_pl_len(&msg->payload.query_hit);
    }

    msg->header.pl_len = pl_len;

    return 0;
}

int _gnut_build_msg_payload(gnut_msg_t *msg, unsigned char *payload,
    uint32_t pl_len) {
    
    unsigned char type;

    type = msg->header.type;

    if(type == 0x00) {
        /* Ping, No Payload, nothing to parse */
    } else if(type == 0x01) {
        if (_gnut_build_pong_msg_payload(&msg->payload.pong, payload) != 0)
            return -1;
        /* Pong, is payload */
    } else if(type == 0x02) {
        /* Bye, is payload */
    } else if(type == 0x40) {
        /* Push, is payload */
    } else if(type == 0x80) {
        /* Query, is payload */
        if (_gnut_build_query_msg_payload(&msg->payload.query, payload) != 0)
            return -3;
    } else if(type == 0x81) {
        if (_gnut_build_query_hit_msg_payload(&msg->payload.query_hit, payload) != 0)
            return -2;
        /* Query Hit, is payload */
    }
    
    return 0;
}

void gnut_dump_msg(const gnut_msg_t *msg) {
    unsigned char type;

    type = msg->header.type;
    
    printf("--== Message Dump Beginning ==--\n");
    printf("Message Type: 0x%.2x.\n", msg->header.type);
    printf("Message TTL: %d.\n", (unsigned int)msg->header.ttl);
    printf("Message Hops: %d.\n", (unsigned int)msg->header.hops);
    printf("Message Payload Len: %d.\n", msg->header.pl_len);

    if(type == 0x00) {
        /* Ping, No Payload, nothing to parse */
    } else if(type == 0x01) {
        /* Pong, is payload */
    } else if(type == 0x02) {
        /* Bye, is payload */
        _gnut_dump_bye_msg_payload(&msg->payload.bye);
    } else if(type == 0x40) {
        /* Push, is payload */
    } else if(type == 0x80) {
        /* Query, is payload */
        _gnut_dump_query_msg_payload(&msg->payload.query);
    } else if(type == 0x81) {
        /* Query Hit, is payload */
    }
    printf("--== Message Dump End ==--\n");
}

int init_handshake(int fd) {
    char hdr[] = "GNUTELLA CONNECT/0.6\r\nUser-Agent: LimeWire/1.0\r\nX-Ultrapeer: True\r\n\r\n";
    char finish_init[] ="GNUTELLA/0.6 200 OK\r\n\r\n";
    int retval = 0;
    char recv_buff[BUFSIZE];
    char tempbuf[BUFSIZE];
    char *templine;
    char *line;

    printf("--attempting to write\n");

    write(fd, (const void *)hdr, strlen(hdr));

    printf("Wrote (%s)\n", hdr);

    //retval = read(fd, (char *)recv_buff, BUFSIZE-1);
    retval = recv(fd, (char *)recv_buff,BUFSIZE-1,0);
    recv_buff[retval] = '\0';

    printf("retval: %d\n",retval);


    printf("Read (%s).\n", recv_buff);

    strcpy(tempbuf,recv_buff);

    line = strtok(tempbuf, "\r\n");

    if((templine = strstr(line,"200")) == NULL) {
        printf("They said no thanks\n");
        return 0;
    } else {
        while ( (line=strtok(NULL, "\r\n")) != NULL)
        {
            printf("RECV LINE: %s\n", line);
        }

        //write back GNUTELLA/0.6 200 OK

        printf("--attempting to write: %s\n",finish_init);
        write(fd, (const void *)finish_init, strlen(finish_init));

    }

	return 1;
}
