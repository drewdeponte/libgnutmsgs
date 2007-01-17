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

#ifndef GNUT_MSGS_H
#define GNUT_MSGS_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "gnut_common.h"
#include "gnut_ping_msg.h"
#include "gnut_pong_msg.h"
#include "gnut_query_msg.h"
#include "gnut_query_hit_msg.h"
#include "gnut_bye_msg.h"

/**
 * Gnutella Message Header
 *
 * A structure designed to represent a Gnutella Message Header.
 */
typedef struct gnut_msg_hdr {
    unsigned char message_id[GNUT_MSG_ID_LEN];  /* Msg ID */
    unsigned char type;                         /* Payload Type */
    unsigned char ttl;                          /* TTL (Time To Live) */
    unsigned char hops;                         /* Hops */
    uint32_t pl_len;                            /* Payload Length */
} gnut_msg_hdr_t;

/**
 * Gnutella Message
 *
 * A structure designed to represent a Gnutella Message.
 */
typedef struct gnut_msg {
    gnut_msg_hdr_t header;      /* Message Header */
    union gnut_msg_spec_payload {
        struct gnut_ping_payload ping;
        struct gnut_pong_payload pong;
        struct gnut_query_payload query;
        struct gnut_query_hit_payload query_hit;
        struct gnut_bye_payload bye;
    } payload;
} gnut_msg_t;

/**
 * Build a Gnutella Message ID
 *
 * Build a Message ID (16 byte) consisting of bytes which are all
 * assigned random values, except for byte 8 which has a value of 0xff
 * and byte 15 which has a value of 0x00. Note: The bytes are numbered
 * 0-15.
 * \param Pointer to message header to store the new message id in.
 * \return An integer representing success (zero) or failure (non-zero).
 * \retval 0 Successfully built gnutella unique message id.
 */
int gnut_build_msg_id(gnut_msg_hdr_t *p_header);

/**
 * Build a Gnutella Message Header
 *
 * Build a Gnutella Message Header given the message payload type, and
 * payload length.
 * \param p_header Pointer to header to store build results in.
 * \param type Payload Type, one of the following values (0x00 = Ping,
 * 0x01 = Pong, 0x02 = Bye, 0x40 = Push, 0x80 = Query, 0x81 = Query Hit)
 * \param pl_len The length of the payload in bytes.
 * \return An integer representing success (zero) or failure (non-zero).
 * \retval 0 Successfully built gnutella message header.
 * \retval -1 Failed to build unique message id.
 */
int gnut_build_msg_hdr(gnut_msg_hdr_t *p_header, unsigned char type,
    uint32_t pl_len);

/**
 * Build a Gnutella Message Header
 *
 * Build a Gnutella Message header given the message id, payload type,
 * and payload length.
 * \param p_header Pointer to header to store build results in.
 * \param message_id Pointer to message id to use in the header.
 * \param type Payload Type, one of the following values (0x00 = Ping,
 * 0x01 = Pong, 0x02 = Bye, 0x40 = Push, 0x80 = Query, 0x81 = Query Hit)
 * \return An integer representing success (zero) or failure (non-zero).
 * \retval 0 Successfully built gnutella message header.
 */
int gnut_build_msg_hdr_given_msg_id(gnut_msg_hdr_t *p_header,
    const unsigned char *message_id, unsigned char type, uint32_t pl_len);

/**
 * Build a Gnutella Ping Message.
 *
 * Build a Gnutella Ping Message so that it may be sent to another
 * servent.
 * \param p_msg Pointer to gnut_msg_t structure to store ping message in.
 * \return An integer representing success (zero) or failure (non-zero).
 * \retval 0 Successfully build guntella ping message.
 * \retval -1 Failed to build message header.
 */
int gnut_build_ping_msg(gnut_msg_t *p_msg);

/**
 * Build a Gnutella Pong Message.
 *
 * Build a Gnutella Pong Message so that it may be sent to another
 * servent.
 * \param p_msg Pointer to gunt_msg_t structure to store pong message in.
 * \param msg_id Message ID of the ping msg this pong is a response to.
 * \param ip_addr C-String dotted quad representation of IP address.
 * \param port_num The port number to tell servents to connect to.
 * \param num_shared_files Num of files are being shared.
 * \param kd_shared Total num of kilobytes that are being shared.
 * \return An integer representing success (zero) or failure (non-zero).
 * \retval 0 Successfully build guntella pong message.
 * \retval -1 Failed to build message header.
 * \retval -2 Failed to convert IP address to proper format.
 */
int gnut_build_pong_msg(gnut_msg_t *p_msg, const unsigned char *msg_id,
    const char *ip_addr, uint16_t port_num, uint32_t num_shared_files,
    uint32_t kb_shared);

/**
 * Build a Gnutella Query Hit Message.
 *
 * Build a Gnutella Query Hit Message so that it may be sent to another
 * servent.
 * \param p_msg Pointer to gnut_msg_t struct to store query hit msg in.
 * \param num_hits Number of hits within the result set of this msg.
 * \param port_num Port number responding node accepts file reqs on.
 * \param ip_addr C-String dotted quad representation of IP address.
 * \param speed Speed of responding node in kb/second.
 * \param list Pointer to linked list of query hit results.
 * \param extra_block Pointer to query hit extra block struct.
 * \param priv_data Priv data (NULL if none specified).
 * \param priv_data_len Length of private data (0 if priv data is NULL).
 * \param servent_id Pointer to the servent id to use for the message.
 */
int gnut_build_query_hit_msg(gnut_msg_t *p_msg, const unsigned char *msg_id,
    unsigned char num_hits, uint16_t port_num, const char *ip_addr,
    uint32_t speed, gnut_query_hit_result_t *list,
    struct gnut_query_hit_extra_block *extra_block,
    unsigned char *priv_data, uint32_t priv_data_len,
    unsigned char *servent_id);

/**
 * Send a Gnutella Message.
 *
 * Send a Gnutella Message given the message and the socket descriptor
 * to send the message over.
 * \param p_msg Pointer to gnut_msg_t structure holding the message.
 * \param sd The file descriptor of the socket to send the message over.
 * \return An integer representing success (zero) or failure (non-zero).
 * \retval 0 Successfully sent the gnutella message.
 * \retval -1 Error, Failed to allocate memory to build wire message.
 * \retval -2 Error, Failed to write message to socket descriptor.
 * \retval -3 Warning, the entire message was NOT sent.
 * \retval -4 Warning, more bytes than exist in the message were sent.
 * \retval -5 Error, Unknown byte order in use.
 * \retval -6 Error, failed to update message payload length.
 */
int gnut_send_msg(gnut_msg_t *p_msg, int sd);

/**
 * Receive a Gnutella Message.
 *
 * Receive a Gnutella Message given a pointer to a message structure to
 * store the received message in, nad a socket descriptor to recv the
 * message on. Note: This function dynamically allocates memory for the
 * payload within the gnut_msg_t structure. Hence, when done with a
 * message one should use the gnut_free_msg() function to free the
 * previously dynamically allocated memory for a given message. Also,
 * note that this function does NOT parse the payload of the message in
 * any way.
 * \param p_msg Pointer to gnut_msg_t structure to hold the message.
 * \param sd The file descriptor of the socket to recv the message on.
 * \return An integer representing success (zero) or failure (non-zero).
 * \retval 0 Successfully received a gnutella message.
 * \retval -1 Error, Unknown byte order in use.
 * \retval -2 Failed to read msg header data from the socket descriptor.
 * \retval -3 Failed to allocate memory for the payload.
 * \retval -4 Failed to read payload data from the socket descriptor.
 * \retval -5 Error, socket closed in mid of reading msg header.
 * \retval -6 Error, socket closed in mid of reading msg payload.
 * \retval -7 Failed to parse message payload.
 */
int gnut_recv_msg(gnut_msg_t *p_msg, int sd);

/**
 * Free a Gnutella Message
 *
 * Free a Gnutella Message that has previously been received via the
 * gnut_recv_msg() function or that has had it's payload built.
 */
void gnut_free_msg(gnut_msg_t *p_msg);

int _gnut_parse_msg_payload(gnut_msg_t *msg, unsigned char *payload,
    uint32_t pl_len);

int _gnut_update_msg_pl_len(gnut_msg_t *msg);

int _gnut_build_msg_payload(gnut_msg_t *msg, unsigned char *payload,
    uint32_t pl_len);

void gnut_dump_msg(const gnut_msg_t *msg);

/**
 * Initialize a Connection to a gnutella server
 * 
 * 
 */
int init_handshake(int fd);

#endif
