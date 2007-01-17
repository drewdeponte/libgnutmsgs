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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "gnut_msgs.h"

unsigned char servent_id[] = {0x54, 0x84, 0x96, 0x0E, 0x06, 0xc8, 0xa9, 0xf5, 0x4a, 0x0d, 0x42, 0xfc, 0xd4, 0x34, 0xd9, 0x00};
/* urn:sha1: */
unsigned char exten_block[] = {
/* HUGE extension */
/* u     r     n     :     s     h     a     1     : */
0x75, 0x72, 0x6e, 0x3a, 0x73, 0x68, 0x61, 0x31, 0x3a,
/* 32 byte base32 encoded (20 byte sha1 hash). */
//0x4d, 0x45, 0x45, 0x47, 0x32, 0x33, 0x41, 0x4c, 0x33, 0x36, 0x42, 0x44, 0x4a,
//0x4e, 0x42, 0x42, 0x4b, 0x49, 0x4d, 0x43, 0x53, 0x49, 0x34, 0x42, 0x42, 0x43,
//0x50, 0x50, 0x57, 0x34, 0x41, 0x50,
// Working fionna apple one we jacked
//0x32, 0x36, 0x52, 0x4f, 0x32, 0x50, 0x58, 0x34, 0x46, 0x43, 0x4b, 0x59, 0x43,
//0x37, 0x43, 0x34, 0x45, 0x5a, 0x55, 0x33, 0x52, 0x33, 0x47, 0x4b, 0x51, 0x47,
//0x50, 0x41, 0x54, 0x36, 0x55, 0x51,
// Previous test working one
//0x4d, 0x45, 0x45, 0x47, 0x32, 0x33, 0x41, 0x4c, 0x33, 0x36, 0x42, 0x44, 0x4a,
//0x4e, 0x42, 0x42, 0x4b, 0x49, 0x4d, 0x43, 0x53, 0x49, 0x34, 0x42, 0x42, 0x43,
//0x50, 0x50, 0x57, 0x34, 0x41, 0x50,
// default1.mp3 hash
0x4e, 0x48, 0x4c, 0x49, 0x34, 0x4b, 0x49, 0x4f, 0x55, 0x57, 0x41, 0x59, 0x59,
0x50, 0x53, 0x35, 0x36, 0x4d, 0x49, 0x4f, 0x5a, 0x42, 0x55, 0x45, 0x41, 0x37,
0x42, 0x4f, 0x47, 0x32, 0x33, 0x4c,
/* extension seperator */
0x1c,
/* GGEP extension content */
/*       2     C     T */
0xc3, 0x82, 0x43, 0x54, 0x44, 0xeb, 0x9f, 0x56, 0x42,
/* null byte to terminate it it as a c-string */
0x00};

/* bad bytes (well just the raw sha1 hash before base32 encoding
0x61, 0x08, 0x6d, 0x6c, 0x0b, 0xdf, 0x82, 0x34, 0xb4, 0x21, 0x52, 0x18, 0x29,
0x23, 0x81, 0x08, 0x9e, 0xfb, 0x70, 0x0f, 0x00}; */
/* extra bytes to append to exten_block
 * 0x1c,0xc3,0x82,0x43,0x54,0x44,0xeb,0x9f,0x56,0x42 */
unsigned char g_priv_data[] = {
0x01, 0xc3, 0x82, 0x42, 0x48, 0x40, 0x00, 0x54
};

int main(int argc, char *argv[]) {
    int sd;
    struct sockaddr_in servaddr;
    gnut_query_hit_result_t *res_set;
	int retval = 0;
    int handshake_ret = 0;
    int r;
    unsigned char *priv_data;

    struct gnut_query_hit_extra_block extra_block;
	gnut_msg_t msg;
	gnut_msg_t my_msg;

    extra_block.ven_code[0] = 'L';
    extra_block.ven_code[1] = 'I';
    extra_block.ven_code[2] = 'M';
    extra_block.ven_code[3] = 'E';
    /*
    extra_block.open_data = (unsigned char *)malloc(4);
    if (extra_block.open_data == NULL) {
        return 1;
    }
    extra_block.open_data_size = 2;
    extra_block.open_data[0] = 0x3d;
    //extra_block.open_data[1] = 0x31;
    extra_block.open_data[1] = 0x11;
    extra_block.open_data[2] = 0x01;
    extra_block.open_data[3] = 0x00;
    */

    if (argc < 3) {
        printf("Usage: %s <ip addr dot quad> <port>\n", argv[0]);
        return 1;
    }

    sd = socket(AF_INET, SOCK_STREAM, 0);
    if (sd == -1) {
        perror("socket");
        return -1;
    }

    printf("--created a socket\n");

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(atoi(argv[2]));
    if (inet_pton(AF_INET, (const char *)argv[1], &servaddr.sin_addr) <= 0) {
        perror("inet_pton");
        return -2;
    }

    printf("--setup the server struct\n");
    printf("--attempting to connect\n");

    retval = connect(sd, (struct sockaddr *)&servaddr, sizeof(servaddr));
    if (retval == -1) {
        perror("connect");
        return -3;
    }

    printf("--finished connect call\n");

	handshake_ret = init_handshake(sd);

	if(handshake_ret == 0) {
		printf("init handshake was bad\n");
	}

    gnut_build_ping_msg(&my_msg);

    gnut_send_msg(&my_msg, sd);

    gnut_free_msg(&my_msg);
                
    while (1) {
        r = gnut_recv_msg(&msg, sd);
        if (r != 0) {
            printf("Err(%d): Fuck, gnut_recv_msg() failed.\n", r);
            break;
        }

        /* Do anything we want with the data from the received message */
        gnut_dump_msg(&msg);

        if (msg.header.type == 0x00) {
            gnut_build_pong_msg(&my_msg, msg.header.message_id,
                "64.214.203.152", 5555, 20, 500000);
            gnut_send_msg(&my_msg, sd);
            gnut_free_msg(&my_msg);
        } else if (msg.header.type == 0x02) {
            /* Got Bye message */
            break;
        } else if (msg.header.type == 0x80) {
            if (strncmp(msg.payload.query.search_str, "cyph", 4) == 0) {
                extra_block.open_data = (unsigned char *)malloc(4);
                if (extra_block.open_data == NULL) {
                    return 1;
                }
                extra_block.open_data_size = 4;
                extra_block.open_data[0] = 0x3c;
                extra_block.open_data[1] = 0x31;
                //extra_block.open_data[1] = 0x11;
                extra_block.open_data[2] = 0x01;
                extra_block.open_data[3] = 0x00;
                priv_data = (unsigned char *)malloc(4);
                if (priv_data == NULL) {
                    free(extra_block.open_data);
                    return 2;
                }
                memcpy((void *)priv_data, (const void *)g_priv_data, 8);
                res_set = create_empty_query_hit_result_set();
                printf("-- created empty query hit result set.\n");

                res_set = append_query_hit_result_to_set(res_set, 5,
                    5910613, "Are you there Josh.mp3", exten_block);
                if (res_set == NULL) {
                    gnut_free_msg(&my_msg);
                    printf("Shit: Append Query Hit Result failed.\n");
                    return 5;
                }
                printf("-- appended query hit result to set.\n");

                retval = gnut_build_query_hit_msg(&my_msg,
                    msg.header.message_id, 1,
                    5600, "207.210.233.110", 5000, res_set, &extra_block,
                    priv_data, 8, servent_id);
                if (retval != 0) {
                    gnut_free_msg(&my_msg);
                    printf("Err: gnut_build_query_hit_msg - failed(%d).\n",
                        retval);
                    return 6;
                }
                printf("-- built query hit msg.\n");

                retval = gnut_send_msg(&my_msg, sd);
                if (retval != 0) {
                    gnut_free_msg(&my_msg);
                    printf("Err: gnut_send_msg - failed(%d).\n", retval);
                    return 7;
                }
                printf("-- sent msg.\n");

                gnut_free_msg(&my_msg);
                printf("-- freed msg.\n");
                
                extra_block.open_data = (unsigned char *)malloc(4);
                if (extra_block.open_data == NULL) {
                    return 1;
                }
                extra_block.open_data_size = 4;
                extra_block.open_data[0] = 0x3c;
                extra_block.open_data[1] = 0x31;
                //extra_block.open_data[1] = 0x11;
                extra_block.open_data[2] = 0x01;
                extra_block.open_data[3] = 0x00;
                priv_data = (unsigned char *)malloc(4);
                if (priv_data == NULL) {
                    free(extra_block.open_data);
                    return 2;
                }
                memcpy((void *)priv_data, (const void *)g_priv_data, 8);
                res_set = create_empty_query_hit_result_set();
                printf("-- created empty query hit result set.\n");

                res_set = append_query_hit_result_to_set(res_set, 5,
                    5910613, "Are you there Josh.mp3", exten_block);
                if (res_set == NULL) {
                    gnut_free_msg(&my_msg);
                    printf("Shit: Append Query Hit Result failed.\n");
                    return 5;
                }
                printf("-- appended query hit result to set.\n");

                retval = gnut_build_query_hit_msg(&my_msg,
                    msg.header.message_id, 1,
                    5601, "207.210.233.111", 5000, res_set, &extra_block,
                    priv_data, 8, servent_id);
                if (retval != 0) {
                    gnut_free_msg(&my_msg);
                    printf("Err: gnut_build_query_hit_msg - failed(%d).\n",
                        retval);
                    return 6;
                }
                printf("-- built query hit msg.\n");

                retval = gnut_send_msg(&my_msg, sd);
                if (retval != 0) {
                    gnut_free_msg(&my_msg);
                    printf("Err: gnut_send_msg - failed(%d).\n", retval);
                    return 7;
                }
                printf("-- sent msg.\n");

                gnut_free_msg(&my_msg);
                printf("-- freed msg.\n");
                
                extra_block.open_data = (unsigned char *)malloc(4);
                if (extra_block.open_data == NULL) {
                    return 1;
                }
                extra_block.open_data_size = 4;
                extra_block.open_data[0] = 0x3c;
                extra_block.open_data[1] = 0x31;
                //extra_block.open_data[1] = 0x11;
                extra_block.open_data[2] = 0x01;
                extra_block.open_data[3] = 0x00;
                priv_data = (unsigned char *)malloc(4);
                if (priv_data == NULL) {
                    free(extra_block.open_data);
                    return 2;
                }
                memcpy((void *)priv_data, (const void *)g_priv_data, 8);
                res_set = create_empty_query_hit_result_set();
                printf("-- created empty query hit result set.\n");

                res_set = append_query_hit_result_to_set(res_set, 5,
                    5910613, "Are you there Josh.mp3", exten_block);
                if (res_set == NULL) {
                    gnut_free_msg(&my_msg);
                    printf("Shit: Append Query Hit Result failed.\n");
                    return 5;
                }
                printf("-- appended query hit result to set.\n");

                retval = gnut_build_query_hit_msg(&my_msg,
                    msg.header.message_id, 1,
                    5602, "207.210.233.112", 5000, res_set, &extra_block,
                    priv_data, 8, servent_id);
                if (retval != 0) {
                    gnut_free_msg(&my_msg);
                    printf("Err: gnut_build_query_hit_msg - failed(%d).\n",
                        retval);
                    return 6;
                }
                printf("-- built query hit msg.\n");

                retval = gnut_send_msg(&my_msg, sd);
                if (retval != 0) {
                    gnut_free_msg(&my_msg);
                    printf("Err: gnut_send_msg - failed(%d).\n", retval);
                    return 7;
                }
                printf("-- sent msg.\n");

                gnut_free_msg(&my_msg);
                printf("-- freed msg.\n");
                
                extra_block.open_data = (unsigned char *)malloc(4);
                if (extra_block.open_data == NULL) {
                    return 1;
                }
                extra_block.open_data_size = 4;
                extra_block.open_data[0] = 0x3c;
                extra_block.open_data[1] = 0x31;
                //extra_block.open_data[1] = 0x11;
                extra_block.open_data[2] = 0x01;
                extra_block.open_data[3] = 0x00;
                priv_data = (unsigned char *)malloc(4);
                if (priv_data == NULL) {
                    free(extra_block.open_data);
                    return 2;
                }
                memcpy((void *)priv_data, (const void *)g_priv_data, 8);
                res_set = create_empty_query_hit_result_set();
                printf("-- created empty query hit result set.\n");

                res_set = append_query_hit_result_to_set(res_set, 5,
                    5910613, "Are you there Josh.mp3", exten_block);
                if (res_set == NULL) {
                    gnut_free_msg(&my_msg);
                    printf("Shit: Append Query Hit Result failed.\n");
                    return 5;
                }
                printf("-- appended query hit result to set.\n");

                retval = gnut_build_query_hit_msg(&my_msg,
                    msg.header.message_id, 1,
                    5603, "207.210.233.113", 5000, res_set, &extra_block,
                    priv_data, 8, servent_id);
                if (retval != 0) {
                    gnut_free_msg(&my_msg);
                    printf("Err: gnut_build_query_hit_msg - failed(%d).\n",
                        retval);
                    return 6;
                }
                printf("-- built query hit msg.\n");

                retval = gnut_send_msg(&my_msg, sd);
                if (retval != 0) {
                    gnut_free_msg(&my_msg);
                    printf("Err: gnut_send_msg - failed(%d).\n", retval);
                    return 7;
                }
                printf("-- sent msg.\n");

                gnut_free_msg(&my_msg);
                printf("-- freed msg.\n");
            }
        }

        gnut_free_msg(&msg);
    }
	
    retval = close(sd);
    if (retval) {
        perror("close");
        return -4;
    }

    printf("--closed connection\n");
	
    return 0;
}

