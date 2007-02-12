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

#include <pthread.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <signal.h>

#include "gnut_msgs.h"

#define SERV_LIST_PORT 4567
#define BACKLOG_MAX 10
#define BUFSIZE 2048
#define MAX_TRYCONNS 10
#define MAX_CONNS 8
#define MAX_IDS 20
#define TIMEOUT 15

#define IP_ADDR_STR_LEN 16

struct ac_sdata {
    char init_ip_addr[IP_ADDR_STR_LEN];
    uint16_t init_port;
};

struct tc_sdata {
    char ip_addr[IP_ADDR_STR_LEN];
    uint16_t port;
};

struct serv_list_node {
    char ip_addr[IP_ADDR_STR_LEN];
    uint16_t port;
    struct serv_list_node *next;
};
struct serv_list_node *serv_list;
pthread_mutex_t serv_list_mutex = PTHREAD_MUTEX_INITIALIZER;


/* Variable and it's associated mutex and condition used to keep track
 * of the number of completed and functioning connections */
int conns_cnt;
pthread_mutex_t conns_cnt_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t conns_cnt_cond = PTHREAD_COND_INITIALIZER;

/* Varialbe and it's associated mutex and condition used to keep track
 * of the number of serv ip and port entries in the servers list */
int serv_list_len;
pthread_mutex_t serv_list_len_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t serv_list_len_cond = PTHREAD_COND_INITIALIZER;

/* Variable and it's associated mutex to keep track of the current
 * number of try_connect threads in existance in the system */
int try_conns_cnt;
pthread_mutex_t try_conns_cnt_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t try_conns_cnt_cond = PTHREAD_COND_INITIALIZER;

FILE *out_fp;
pthread_mutex_t out_fp_mutex = PTHREAD_MUTEX_INITIALIZER;

struct conn_list_node {
    int sd;
    pthread_mutex_t sd_mutex;
    uint16_t port;
    char ip_addr[16];
    char msg_id_list[MAX_IDS][GNUT_MSG_ID_LEN];
    pthread_mutex_t msg_id_list_mutex;
    struct conn_list_node *next;
};

/* The head of the linked list of connections and it's mutex */
struct conn_list_node *conn_list;
pthread_mutex_t conn_list_mutex = PTHREAD_MUTEX_INITIALIZER;

struct hc_sdata {
    int sd;
    pthread_mutex_t sd_mutex;
};

struct serv_list_node *append_server_to_list(const char *ip_addr, uint16_t port,
    struct serv_list_node *p_list);
struct serv_list_node *prepend_server_to_list(const char *ip_addr, uint16_t port,
    struct serv_list_node *p_list);
int get_server_from_list(char *ip_addr, uint16_t *port,
    struct serv_list_node *p_list);
int get_len_of_serv_list(void);
int handle_handshake(int fd);
void parse_list(char *buf);
void print_list(struct serv_list_node *head);

struct conn_list_node *get_conn_node(int sd);

int bcast_msg(int orig_sd, gnut_msg_t *p_msg);
int fwrd_qhit_msg(gnut_msg_t *p_msg);
int wang_qhit_msg(gnut_msg_t *p_msg);
int rem_conn_from_list(int sd);
int add_conn_to_list(int sd, uint16_t port, char *ip_addr);
void print_conn_list(void);
int prebuild_serv_list(char *filename);

void *handle_conn(void *arg) {

    struct hc_sdata *p_data;
    int sd;
    gnut_msg_t rmsg;
    gnut_msg_t mymsg;
    //char tmp_ip[16];
	//struct serv_list_node *tmphead;
    int r;
    struct conn_list_node *p_conn;
    fd_set readfds;

    int last_msg_id_pos, msg_id_list_looped;
    
    last_msg_id_pos = -1;
    msg_id_list_looped = 0;

    printf("handle_conn() CALLED.\n");

    p_data = (struct hc_sdata *)arg;

    sd = p_data->sd;

    /* Here I a get the pointer to the actual connection node given the
     * socket descriptor. */
    p_conn = get_conn_node(sd);
    if (p_conn == NULL) {
        printf("SHITFUCK! get_conn_node() failed.\n");
    }

    /* Here I build the intial ping message to make after I make a
     * connection to another gnutella node. Then I send the ping to the
     * gnutella node. */
    gnut_build_ping_msg(&mymsg);

    fprintf(stderr, "hand_conn(%d): about to lock sd_mutex.\n", sd);
    pthread_mutex_lock(&p_conn->sd_mutex);
    fprintf(stderr, "hand_conn(%d): locked sd_mutex.\n", sd);
    r = gnut_send_msg(&mymsg, sd);
    if (r != 0) {
        fprintf(stderr, "hand_conn(%d): about to unlock sd_mutex.\n", sd);
        pthread_mutex_unlock(&p_conn->sd_mutex);
        fprintf(stderr, "hand_conn(%d): unlocked sd_mutex.\n", sd);
        fprintf(stderr, "Err(%d): About to remove conn from list.\n", sd);
        rem_conn_from_list(sd);
        fprintf(stderr, "Err(%d): Removed conn from list.\n", sd);
        return NULL;
    }
    pthread_mutex_unlock(&p_conn->sd_mutex);
    fprintf(stderr, "hand_conn(%d): unlocked sd_mutex.\n", sd);
    gnut_free_msg(&mymsg);

    FD_ZERO(&readfds);
    FD_SET(sd, &readfds);

    r = select(sd+1, &readfds, NULL, NULL, NULL);
    if (r == -1) {
        perror("select");
        rem_conn_from_list(sd);
        return NULL;
    }
    fprintf(stderr, "DATA IS AVAILABLE for %d.\n", sd);


    fprintf(stderr, "hand_conn(%d)recv1: about to lock sd_mutex.\n", sd);
    pthread_mutex_lock(&p_conn->sd_mutex);
    fprintf(stderr, "hand_conn(%d)recv1: locked sd_mutex.\n", sd);
    r = gnut_recv_msg(&rmsg, sd);
    fprintf(stderr, "hand_conn(%d)recv1: about to unlock sd_mutex.\n", sd);
    pthread_mutex_unlock(&p_conn->sd_mutex);
    fprintf(stderr, "hand_conn(%d)recv1: unlocked sd_mutex.\n", sd);
    while (r == 0) {
        printf("Received message on sd (%d).\n", sd);
        //gnut_dump_msg(&rmsg);

        if (rmsg.header.type == 0x00) {
            // ping - respond to the pings with pongs pongs pongs
            if ((rmsg.header.ttl == 1) && ((rmsg.header.hops == 0) || (rmsg.header.hops == 1))) {
                /* received a probe ping must respond with pong about
                 * me. */
                printf("--RECV'd Probe Ping.\n");
                gnut_build_pong_msg(&mymsg, rmsg.header.message_id,
                    "75.111.17.120", 4567, 20, 500000);
                pthread_mutex_lock(&p_conn->sd_mutex);
                r = gnut_send_msg(&mymsg, sd);
                if (r != 0) {
                    printf("FUCK GNUT_SEND_MSG() FAILED in Probe Ping.\n");
                    pthread_mutex_unlock(&p_conn->sd_mutex);
                    gnut_free_msg(&mymsg);
                    rem_conn_from_list(sd);
                    return NULL;
                }
                pthread_mutex_unlock(&p_conn->sd_mutex);
                gnut_free_msg(&mymsg);
                printf("--SENT Pong.\n");
            } else if ((rmsg.header.ttl == 2) && (rmsg.header.hops == 0)) {
                /* received a "Crawling Ping" which is used to scan the
                 * network, It SHOULD be replied to with pongs
                 * containing information about the host receiving the
                 * ping and all other hosts it is connected to. The
                 * information about neighbour nodes can be provided by
                 * either creating pongs on their behalf or by
                 * forwarding the pings and then forwarding the pongs
                 * back. */
                printf("--RECV'd \"Crawling Ping\".\n");
                gnut_build_pong_msg(&mymsg, rmsg.header.message_id,
                    "75.111.17.120", 4567, 20, 500000);
                pthread_mutex_lock(&p_conn->sd_mutex);
                gnut_send_msg(&mymsg, sd);
                if (r != 0) {
                    printf("FUCK GNUT_SEND_MSG() FAILED in Crawling Ping.\n");
                    pthread_mutex_unlock(&p_conn->sd_mutex);
                    gnut_free_msg(&mymsg);
                    rem_conn_from_list(sd);
                    return NULL;
                }
                pthread_mutex_unlock(&p_conn->sd_mutex);
                gnut_free_msg(&mymsg);
                printf("--SENT Pong.\n");
            } else {
                printf("--RECV'd General Ping.\n");
                gnut_build_pong_msg(&mymsg, rmsg.header.message_id,
                    "75.111.17.120", 4567, 20, 500000);
                pthread_mutex_lock(&p_conn->sd_mutex);
                gnut_send_msg(&mymsg, sd);
                if (r != 0) {
                    printf("FUCK GNUT_SEND_MSG() FAILED in General Ping.\n");
                    pthread_mutex_unlock(&p_conn->sd_mutex);
                    gnut_free_msg(&mymsg);
                    rem_conn_from_list(sd);
                    return NULL;
                }
                pthread_mutex_unlock(&p_conn->sd_mutex);
                gnut_free_msg(&mymsg);
                printf("--SENT Pong.\n");
               
                r = bcast_msg(sd, &rmsg);
                if (r != 0) {
                    printf("BCAST FAILED.\n");
                    rem_conn_from_list(sd);
                    return NULL;
                }
                printf("--broadcast ping to connected servers.\n");
            }
        } else if (rmsg.header.type == 0x01) {
            // pong - parse the ip address and ports and add them to
            // serv list
            printf("GOT PONG FROM server.\n");
            /*
            inet_ntop(AF_INET, (const void *)&rmsg.payload.pong.ip_addr, 
                tmp_ip, 16);
            tmp_ip[15] = '\0';
            pthread_mutex_lock(&serv_list_mutex);
            if((tmphead = append_server_to_list(tmp_ip,
                (uint16_t)rmsg.payload.pong.port_num,serv_list)) != NULL) {

                serv_list = tmphead;
            }
            pthread_mutex_unlock(&serv_list_mutex);
            pthread_mutex_lock(&serv_list_len_mutex);
            serv_list_len++;
            pthread_mutex_unlock(&serv_list_len_mutex);
            printf("--added server to serv list.\n");
            */
        } else if (rmsg.header.type == 0x02) {
            printf("Recv'd BYE msg.\n");
            // bye
            break;
        } else if (rmsg.header.type == 0x80) {
            printf("GOT QUERY FROM SERVER\n");
            //gnut_dump_msg(&rmsg);
            // query
            /*
            r = bcast_msg(sd, &rmsg);
            if (r != 0) {
                rem_conn_from_list(sd);
                return NULL;
            }
            printf("--broadcast query to connected servers.\n");
            if (!msg_id_list_looped) {
                if (last_msg_id_pos == -1) {
                    last_msg_id_pos = 0;
                    memcpy(p_conn->msg_id_list[last_msg_id_pos],
                        rmsg.header.message_id, GNUT_MSG_ID_LEN);
                } else {
                    if (last_msg_id_pos < (MAX_IDS - 1)) {
                        last_msg_id_pos += 1;
                        memcpy(p_conn->msg_id_list[last_msg_id_pos],
                            rmsg.header.message_id, GNUT_MSG_ID_LEN);
                    } else {
                        last_msg_id_pos = 0;
                        msg_id_list_looped = 1;
                        memcpy(p_conn->msg_id_list[last_msg_id_pos],
                            rmsg.header.message_id, GNUT_MSG_ID_LEN);
                    }
                }
            } else {
                if (last_msg_id_pos < (MAX_IDS - 1)) {
                    last_msg_id_pos += 1;
                    memcpy(p_conn->msg_id_list[last_msg_id_pos],
                        rmsg.header.message_id, GNUT_MSG_ID_LEN);
                } else {
                    last_msg_id_pos = 0;
                    msg_id_list_looped = 1;
                    memcpy(p_conn->msg_id_list[last_msg_id_pos],
                        rmsg.header.message_id, GNUT_MSG_ID_LEN);
                }
            }
            printf("--stored query message id in pos[%d]", last_msg_id_pos);
            */
        } else if (rmsg.header.type == 0x81) {
            // query hit message
            printf("GOT A FUCKING QUERY HIT!!!!!!!!!!\n");
            //fwrd_qhit_msg(&rmsg);
            printf("FORWARDED QUERY HIT MSG.\n");
        } else {
            printf("GOT SOME OTHER TYPE OF MESSAGE.\n");
        }

        gnut_free_msg(&rmsg);
        //printf("-freed the message.\n");
    
        r = select(sd+1, &readfds, NULL, NULL, NULL);
        if (r == -1) {
            perror("select");
            rem_conn_from_list(sd);
            return NULL;
        }
    
        printf("DATA IS AVAILABLE for %d.\n", sd);

        /*
        pthread_mutex_lock(&p_conn->sd_mutex);
        r = gnut_recv_msg(&rmsg, sd);
        pthread_mutex_unlock(&p_conn->sd_mutex);
        */

        fprintf(stderr, "hand_conn(%d)recv2: about to lock sd_mutex.\n", sd);
        pthread_mutex_lock(&p_conn->sd_mutex);
        fprintf(stderr, "hand_conn(%d)recv2: locked sd_mutex.\n", sd);
        r = gnut_recv_msg(&rmsg, sd);
        fprintf(stderr, "hand_conn(%d)recv2: about to unlock sd_mutex.\n", sd);
        pthread_mutex_unlock(&p_conn->sd_mutex);
        fprintf(stderr, "hand_conn(%d)recv2: unlocked sd_mutex.\n", sd);
    }

    fprintf(stderr, "hand_conn(%d): about to rem conn from list and exit.\n", sd);

    rem_conn_from_list(sd);

    free(p_data);
    return NULL;
}

void *try_connect(void *arg) {
    char ip_addr[IP_ADDR_STR_LEN];  /* temp storage for serv ip address */
    uint16_t port;                  /* temp storage for serv port */
    struct sockaddr_in servaddr;
    struct tc_sdata *p_data;
    int sockflags, connsockflag;
    socklen_t connsockflag_size;
    struct timeval timeout;
    fd_set writesds;
    int r, sd, retval;
    struct hc_sdata *p_hcsdata;
    pthread_t conn_hand_tid;

    connsockflag_size = sizeof(connsockflag);
   
    /* Get the ip and port from the passed data and free it. */
    p_data = (struct tc_sdata *)arg;
    strncpy(ip_addr, p_data->ip_addr, IP_ADDR_STR_LEN);
    ip_addr[IP_ADDR_STR_LEN - 1] = '\0';
    port = p_data->port;
    free((void *)p_data);
   
    /* Create a TCP socket descriptor. */
    sd = socket(AF_INET, SOCK_STREAM, 0);
    if (sd == -1) {
        printf("try_conn: Err(%d): call to socket() failed.\n", sd);
        perror("socket");
        pthread_mutex_lock(&try_conns_cnt_mutex);
        try_conns_cnt--;
        pthread_cond_signal(&try_conns_cnt_cond);
        pthread_mutex_unlock(&try_conns_cnt_mutex);
        return NULL;
    }

    /* Obtain the current socket flags, set the socket to non-blocking
     * so that I can use the select() function on the socket during a
     * connect() to implement a timeout */
    sockflags = fcntl(sd, F_GETFL, 0);
    fcntl(sd, F_SETFL, sockflags | O_NONBLOCK);
    timeout.tv_sec = TIMEOUT;
    timeout.tv_usec = 0;

    /* Set the server's addr and port information */
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    if (inet_pton(AF_INET, (const char *)ip_addr, &servaddr.sin_addr) <= 0) {
        printf("try_conn: Err: call to inet_pton() failed.\n");
        perror("inet_pton");
        pthread_mutex_lock(&try_conns_cnt_mutex);
        try_conns_cnt--;
        pthread_cond_signal(&try_conns_cnt_cond);
        pthread_mutex_unlock(&try_conns_cnt_mutex);
        close(sd);
        return NULL;
    }

    /* Clear out the file descriptor set to use with select() and add
     * the new socket descriptor to the set. */
    FD_ZERO(&writesds);
    FD_SET(sd, &writesds);

    /* Attempt to establish a TCP connection to the server. */
    retval = connect(sd, (struct sockaddr *)&servaddr, sizeof(servaddr));
    
	if(!FD_ISSET(sd, &writesds)) { 
		printf("omg this was it\n"); 
		return NULL; 
	}
	
	printf("try_conn: connecting to %s:%d\n", ip_addr, port);
    retval = select(sd+1, NULL, &writesds, NULL, &timeout);
    
	if (retval == 0) { /* hit the timeout */
        printf("try_conn: connecting to %s:%d timed out.\n", ip_addr, port);
        FD_ZERO(&writesds);
        /* fcntl(sd, F_SETFL, sockflags); */
        close(sd);

		printf("before lock in timeout(%d)\n",sd);
        pthread_mutex_lock(&try_conns_cnt_mutex);
        try_conns_cnt--;
		printf("in lock in timeout(%d)\n",sd);
        pthread_cond_signal(&try_conns_cnt_cond);
        pthread_mutex_unlock(&try_conns_cnt_mutex);

		printf("after locks in timeout(%d)\n",sd);

        fcntl(sd, F_SETFL, sockflags);
        close(sd);
        FD_ZERO(&writesds);


        return NULL;
    } else if (retval < 0) {  /* a freakin error */
        printf("try_conn: %s:%d Err(%d): Call to select() failed.\n", ip_addr,
            port, retval);
        pthread_mutex_lock(&try_conns_cnt_mutex);
        try_conns_cnt--;
        pthread_cond_signal(&try_conns_cnt_cond);
        pthread_mutex_unlock(&try_conns_cnt_mutex);
        close(sd);
        return NULL;
    } else { /* call to select succeeded now must see if actually made conn */
		
		if(!FD_ISSET(sd, &writesds)) { 
			printf("omg this was it(2)\n"); 
			return NULL; 
		}

        r = getsockopt(sd, SOL_SOCKET, SO_ERROR, &connsockflag,
            &connsockflag_size);
        if (r != 0) {
            printf("try_conn: %s:%d Err(%d): call to getsockopt() failed.\n",
                ip_addr, port, r);
            perror("getsockopt");
            pthread_mutex_lock(&try_conns_cnt_mutex);
            try_conns_cnt--;
            pthread_cond_signal(&try_conns_cnt_cond);
            pthread_mutex_unlock(&try_conns_cnt_mutex);
            close(sd);
            return NULL;
        }

        if (connsockflag == 0) {
            printf("try_conn: connected to %s:%d\n", ip_addr, port);
            pthread_mutex_lock(&out_fp_mutex);
            fprintf(out_fp, "%s:%d\n", ip_addr, port);
            fflush(out_fp);
            pthread_mutex_unlock(&out_fp_mutex);
        } else {
            printf("try_conn: failed to connect to %s:%d\n", ip_addr, port);
            pthread_mutex_lock(&try_conns_cnt_mutex);
            try_conns_cnt--;
            pthread_cond_signal(&try_conns_cnt_cond);
            pthread_mutex_unlock(&try_conns_cnt_mutex);
            close(sd);
            return NULL;
        }
    }

    fcntl(sd, F_SETFL, sockflags);



    /* Perform the handshake here */
    retval = handle_handshake(sd);
    if (retval == 0) {
        printf("try_conn: %s:%d Handshake rejected, but provided server list.\n", ip_addr, port);
        close(sd);
    } else if (retval == 1) {
        /* successfully performed handshake */
        printf("try_conn: %s:%d Handshake accepted, adding to central conn list.\n", ip_addr, port);
        
        p_hcsdata = (struct hc_sdata *)malloc(sizeof(struct hc_sdata));
        if (p_hcsdata == NULL) {
            printf("try_conn: Err: failed in call to malloc().\n");
            pthread_mutex_lock(&try_conns_cnt_mutex);
            try_conns_cnt--;
            pthread_cond_signal(&try_conns_cnt_cond);
            pthread_mutex_unlock(&try_conns_cnt_mutex);
            close(sd);
            return NULL;
        }
        p_hcsdata->sd = sd;

        pthread_mutex_lock(&conns_cnt_mutex);
        if (conns_cnt < MAX_CONNS) {
            //sockflags = fcntl(sd, F_GETFL, 0);
            //fcntl(sd, F_SETFL, sockflags | O_NONBLOCK);

            pthread_mutex_unlock(&conns_cnt_mutex);
            /* append connection to central connection list and remove
             * the ip_addr from the attempt to connect ip_addr list. */
            r = add_conn_to_list(sd, port, ip_addr);
            if (r != 0) {
                printf("try_conn: %s:%d Err(%d): Failed to add conn to conns list.\n", ip_addr, port, r);
                if ((r = close(sd)) == -1) {
                    printf("try_conn: %s:%d Failed to close(%d).\n",
                        ip_addr, port, sd);
                    perror("close");
                }
                free((void *)p_hcsdata);
            }

            /* create a new handle_conn thread to handle the newly
             * created connection. */
            r = pthread_create(&conn_hand_tid, NULL, handle_conn, p_hcsdata);
            if (r != 0) {
                printf("try_conn: %s:%d Err(%d): Failed to create conn handler.\n",
                    ip_addr, port, r);
                if ((r = close(sd)) == -1) {
                    printf("try_conn: %s:%d Failed to close(%d).\n",
                        ip_addr, port, sd);
                    perror("close");
                }
                free((void *)p_hcsdata);
            }
        } else {
            pthread_mutex_unlock(&conns_cnt_mutex);
        }
    } else if (retval == 2) {
        printf("try_conn(%d): %s:%d Handshake rejecet, without list of servers.\n", sd, ip_addr, port);
        if ((r = close(sd)) == -1) {
            printf("try_conn: %s:%d Failed to close(%d).\n", ip_addr, port,
                sd);
            perror("close");
        }
    } else if (retval == -1) {
        printf("try_conn(%d): %s:%d handle_handshake() failed.\n",
            sd, ip_addr, port);
        if ((r = close(sd)) == -1) {
            printf("try_conn: %s:%d Failed to close(%d).\n", ip_addr, port,
                sd);
            perror("close");
        }
    }

    pthread_mutex_lock(&try_conns_cnt_mutex);
    try_conns_cnt--;
    pthread_cond_signal(&try_conns_cnt_cond);
    pthread_mutex_unlock(&try_conns_cnt_mutex);

    return NULL;
}

void *attempt_connect(void *arg) {
    uint16_t cur_port;
    char cur_ip[IP_ADDR_STR_LEN];
    struct tc_sdata *p_tcsdata;
    pthread_t tc_tids[MAX_TRYCONNS];
    int num_to_try;
    int pop_ret;
    int r, i;

    while (1) {
        /* Check both the empty server list case and the max allowed
         * connections case to see if we need to block waiting for an
         * appropriate signal */
        pthread_mutex_lock(&conns_cnt_mutex);
        if (conns_cnt == MAX_CONNS) {
            printf("Shit conns_cnt == MAX_CONNS, waitin for signal\n");
            pthread_cond_wait(&conns_cnt_cond, &conns_cnt_mutex);
        }
        pthread_mutex_unlock(&conns_cnt_mutex);
        
        pthread_mutex_lock(&try_conns_cnt_mutex);
        if (try_conns_cnt == MAX_TRYCONNS) {
            printf("Shit try_conns_cont == MAX_CONNS, waitin for signal.\n");
            pthread_cond_wait(&try_conns_cnt_cond, &try_conns_cnt_mutex);
        }
        pthread_mutex_unlock(&try_conns_cnt_mutex);
        
        pthread_mutex_lock(&serv_list_len_mutex);
        if (serv_list_len == 0) {
            printf("Shit serv_list_len == 0, watin for signal\n");
            pthread_cond_wait(&serv_list_len_cond, &serv_list_len_mutex);
        }
        pthread_mutex_unlock(&serv_list_len_mutex);
        
        /* Calculate the appropriate number of try conn threads to
         * create */
        pthread_mutex_lock(&try_conns_cnt_mutex);
        num_to_try = MAX_TRYCONNS - try_conns_cnt;
		printf("num to try: %d\n",num_to_try);
        pthread_mutex_unlock(&try_conns_cnt_mutex);

        pthread_mutex_lock(&serv_list_len_mutex);
		printf("serv_list_len: %d\n",serv_list_len);		      
		if (serv_list_len < num_to_try)
            num_to_try = serv_list_len;
		printf("num_to try then after servlistlencheck: %d\n",num_to_try);
        pthread_mutex_unlock(&serv_list_len_mutex);

        /* Iterate from 0 to num_to_try popping a server ip and port off
         * the list and spawning a new try_connect thread for each one
         * storing the thread ids in the tc_tids array. */
        for (i = 0; i < num_to_try; i++) {
            pthread_mutex_lock(&serv_list_mutex);
            pop_ret = get_server_from_list(&cur_ip[0], &cur_port, serv_list);
			printf("pop_ret for _get_server_from_list was: %d\n",pop_ret);
            pthread_mutex_unlock(&serv_list_mutex);
            p_tcsdata = (struct tc_sdata *)malloc(sizeof(struct tc_sdata));
            if (p_tcsdata != NULL) {
                strncpy(p_tcsdata->ip_addr, cur_ip, IP_ADDR_STR_LEN);
                p_tcsdata->port = cur_port;
                r = pthread_create(&tc_tids[i], NULL, try_connect, p_tcsdata);
                if (r != 0) {
                    printf("attempt_conn: Err(%d): pthread_create().\n", r);
                    perror("attempt_conn: pthread_create(try_conn)");
                    free((void *)p_tcsdata);
                } else {
                    pthread_detach(tc_tids[i]);
                    pthread_mutex_lock(&try_conns_cnt_mutex);
					printf("try_conns_cnt: %d\n",try_conns_cnt);
                    try_conns_cnt++;
                    pthread_mutex_unlock(&try_conns_cnt_mutex);
                }
            } else {
                printf("attempt_conn: Err: malloc() failed.\n");
            }
            print_conn_list();
            usleep(3);
        }
    }

    printf("attempt_conn: Exiting thread.\n");
    return NULL;
}

/* The basic logic
 *
 * Basically I make a bunch of connections and listen on each connection
 * for queries, if I get a query then I forward it to all the other
 * connections except the one it came in on, then I wait on the other
 * connections for query hits. If I get a query hit that matches the
 * message id of the query then I change the file size, file id, sha1
 * hash, file server, and file server port, and then forward the query
 * hit on to client that made the original query.
 */

int main(int argc, char *argv[]) {
    int listsd, connsd, reuse_set_flag;
    int r;
    struct sockaddr_in servaddr;
    struct sockaddr_in clntaddr;
    socklen_t len;

    //struct ac_sdata ac_data;
    pthread_t ac_tid;

    conns_cnt = 0;
    serv_list_len = 0;
    try_conns_cnt = 0;
    conn_list = NULL;

    out_fp = fopen("./outultras", "a");
    if (out_fp == NULL) {
        exit(1);
    }

    prebuild_serv_list(argv[1]);
    /*
    strncpy(ac_data.init_ip_addr, argv[1], 16);
    ac_data.init_ip_addr[15] = '\0';
    ac_data.init_port = atoi(argv[2]);
    */

    r = pthread_create(&ac_tid, NULL, attempt_connect, NULL);
    if (r != 0) {
        perror("pthread_create");
        exit(1);
    }

    r = pthread_detach(ac_tid);
    if (r != 0) {
        perror("pthread_detach");
        exit(2);
    }

    /* Below is the listen for connection thread, it is the default
     * thread which will always be running. The attempt to connect
     * thread on the other hand may eventually exaust it's list of IPs
     * and exit. */
    listsd = socket(AF_INET, SOCK_STREAM, 0);
    if (listsd == -1) {
        perror("socket");
        exit(3);
    }

    reuse_set_flag = 1;
    r = setsockopt(listsd, SOL_SOCKET, SO_REUSEADDR,
        (const void *)&reuse_set_flag, (socklen_t)sizeof(reuse_set_flag));
    if (r == -1) {
        perror("setsockopt");
        exit(4);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(SERV_LIST_PORT);

    r = bind(listsd, (struct sockaddr *)&servaddr,
        (socklen_t)sizeof(servaddr));
    if (r == -1) {
        perror("bind");
        exit(5);
    }

    r = listen(listsd, BACKLOG_MAX);
    if (r == -1) {
        perror("listen");
        exit(6);
    }

    while (1) {
        pthread_mutex_lock(&conns_cnt_mutex);
        if (conns_cnt == MAX_CONNS) {
            pthread_cond_wait(&conns_cnt_cond, &conns_cnt_mutex);
        }
        pthread_mutex_unlock(&conns_cnt_mutex);

        memset(&clntaddr, 0, sizeof(clntaddr));
        len = sizeof(clntaddr);
        connsd = accept(listsd, (struct sockaddr *)&clntaddr, &len);
        if (connsd == -1) {
            perror("accept");
            exit(7);
        }
        printf("Accepted connection.\n");

        /* Here should do the recv handshake shit. */

        /* If handshake is successfull then do the following

        r = add_conn_to_list(connsd, cur_port, cur_ip);
        if (r != 0) {
            printf("Failed to add conn to central conns list.\n");
        }
        */

    }

    fclose(out_fp);

    return 0;
}

struct serv_list_node *append_server_to_list(const char *ip_addr,
    uint16_t port, struct serv_list_node *p_list) {

    struct serv_list_node *cur_node;
    struct serv_list_node *prev_node;
    struct serv_list_node *new_node;

    prev_node = p_list;
    cur_node = prev_node;
    while (cur_node) {
        prev_node = cur_node;
        cur_node = cur_node->next;
    }

    //printf("Got ip_addr: %s\n", ip_addr);
    //printf("Got port: %d\n", port);

    /* at this point cur_node should be equal to NULL and prev_node
     * should be the last valid node in the list. Either that or both
     * cur_node and prev_node will be NULL if p_list was equal to NULL.
     * In this case I should create the initial node of the list and
     * return the new list. */
    if ((cur_node == NULL) && (prev_node == NULL) && (p_list == NULL)) {
        //printf("Adding new initial node head.\n");
        /* the list is empty, hence I create the first node of the list
         * and return the pointer to the list */
        new_node = (struct serv_list_node *)malloc(sizeof(struct serv_list_node));
        if (new_node == NULL) {
            perror("malloc");
            return NULL;
        }

        strncpy(new_node->ip_addr, ip_addr, 16);
        new_node->ip_addr[15] = '\0';
        new_node->port = port;
        new_node->next = NULL;

        return new_node;
    } else if ((cur_node == NULL) && (prev_node != NULL) && (p_list != NULL)) {
        //printf("Adding node to end of list.\n");
        /* the list is not empty, hence I append the ip and port to the
         * end of the list and return the pointer to the head of the
         * list. */
        new_node = (struct serv_list_node *)malloc(sizeof(struct serv_list_node));
        if (new_node == NULL) {
            perror("malloc");
            return NULL;
        }

        strncpy(new_node->ip_addr, ip_addr, 16);
        new_node->ip_addr[15] = '\0';
        new_node->port = port;
        new_node->next = NULL;

        prev_node->next = new_node;

        return p_list;
    } else {
        /* wow, something is fucked up with the logic if we get to this
         * point. */
        printf("WOW FUCK, we shouldn't be hitting this.\n");
        return NULL;
    }
}

struct serv_list_node *prepend_server_to_list(const char *ip_addr,
    uint16_t port, struct serv_list_node *p_list) {

    struct serv_list_node *new_node;

    new_node = (struct serv_list_node *)malloc(sizeof(struct serv_list_node));
    if (new_node == NULL) {
        perror("malloc");
        return NULL;
    }

    strncpy(new_node->ip_addr, ip_addr, 16);
    new_node->ip_addr[15] = '\0';
    new_node->port = port;
    new_node->next = p_list;

    p_list = new_node;

    return p_list;
}

int get_server_from_list(char *ip_addr, uint16_t *port,
    struct serv_list_node *p_list) {

    struct serv_list_node *actret_node;

    if (p_list == NULL) {
        return -1;
    }

    actret_node = p_list->next;
    strncpy(ip_addr, p_list->ip_addr, 16);
    *(port) = p_list->port;
    free(p_list);

    serv_list = actret_node;

    pthread_mutex_lock(&serv_list_len_mutex);
    serv_list_len--;
    pthread_cond_signal(&serv_list_len_cond);
    pthread_mutex_unlock(&serv_list_len_mutex);

    return 0;
}

int handle_handshake(int fd) {
    char hdr[] = "GNUTELLA CONNECT/0.6\r\nUser-Agent: Mutella-0.4.5\r\nX-Ultrapeer: True\r\nX-Leaf-Max: 32\r\n\r\n";
    char finish_init[] ="GNUTELLA/0.6 200 OK\r\n\r\n";
    int retval = 0;
    char recv_buff[BUFSIZE];
    char tempbuf[BUFSIZE];
    char *templine;
    char *line;
	char *saveptr1, *saveptr2;
    int bytes_written;
    int r;

    bytes_written = 0;

	//signal (SIGPIPE, SIG_IGN);

    //printf("--attempting to write\n");

    while (bytes_written < strlen(hdr)) {
        r = send(fd, (const void *)hdr, strlen(hdr) - bytes_written, 0);
        if (r < 0) {
            perror("write");
            return -1;
        } else if (r == 0) {
            printf("SHITFUCKME, write() returned 0.\n");
        } else {
            bytes_written += r;
        }
    }

    //write(fd, (const void *)hdr, strlen(hdr));

    //printf("Wrote (%s)\n", hdr);

    retval = recv(fd, (char *)recv_buff,BUFSIZE-1,0);
    if (retval == 0) {
        printf("nigger closed the socket on us.\n");
        return -1;
    } else if (retval < 0) {
        printf("recv had an error (%d).\n", retval);
        fprintf(stderr, "Oh shit, an error.\n");
        perror("recv");
        return -1;
    }
    recv_buff[retval] = '\0';

    //printf("Read (%s).\n", recv_buff);

    strcpy(tempbuf,recv_buff);
    line = strtok_r(tempbuf, "\r\n",&saveptr1);
    if((templine = strstr(line,"200 OK")) == NULL) {
        //printf("They said no thanks, so checking now for list\n");
    } else {
		//printf("They said ok\n");
        //write back GNUTELLA/0.6 200 OK

        //printf("--attempting to write: %s\n",finish_init);
        r = write(fd, (const void *)finish_init, strlen(finish_init));
        if (r < 0) {
            printf("SHITFUCK(2), write() failed with %d.\n", r);
        } else if (r == 0) {
            printf("SHITFUCKME(2), write() returned 0.\n");
        }

		return 1;
    }

	//so we got rejected with a 503, lets go through again and check for X-TRY-Ultrapeers
	memset(tempbuf,0x00,BUFSIZE);
	strcpy(tempbuf,recv_buff);
    line = strtok_r(tempbuf, "\r\n",&saveptr2);
	while ( (line=strtok_r(saveptr2, "\r\n",&saveptr2)) != NULL)     
	{
    	if((templine = strstr(line,"X-Try-Ultrapeers:")) != NULL) {
			//printf("got a list!\n");
			parse_list(recv_buff);

            //printf("head after parse: %p\n", *(head));

			return 0;
    	} else if ((templine = strstr(line, "X-Try:")) != NULL) {
            parse_list(recv_buff);

            return 0;
        }
	}

	return 2;
}

void parse_list(char *buf)
{
	char tempbuf[BUFSIZE];
    char *line;
    char *ip;
	char *start;
	char *saveptr1;
	char *colon;
	char single_ip[16];
	int port;
	struct serv_list_node *tmphead;

	strcpy(tempbuf,buf);
	line = strtok_r(tempbuf, "\r\n",&saveptr1);

    while ( (line=strtok_r(saveptr1, "\r\n",&saveptr1)) != NULL)
    {
		if((start = strstr(line,"X-Try-Ultrapeers:")) != NULL) {
			line = start+=18;
		
			while((ip = index(line,',')) != NULL) {
				*(ip) = '\0';
						
				//this is where we call something to add the ip/port to a list
				colon = index(line,':');
				*(colon) = '\0';
				strcpy(single_ip,line);
				line = colon+1;
				port = atoi(line);
				
				//add me to list -- PUT LIST CODE HERE, add to ComList
                pthread_mutex_lock(&serv_list_mutex);
				if((tmphead = append_server_to_list(single_ip,
                    (uint16_t)port,serv_list)) != NULL) {

					serv_list = tmphead;
			    }
                pthread_mutex_unlock(&serv_list_mutex);
                pthread_mutex_lock(&serv_list_len_mutex);
                serv_list_len++;
                pthread_mutex_unlock(&serv_list_len_mutex);

				line = ip+1;		
			}	
		} else if((start = strstr(line,"X-Try:")) != NULL) {
			line = start+=7;
		
			while((ip = index(line,',')) != NULL) {
				*(ip) = '\0';
						
				//this is where we call something to add the ip/port to a list
				colon = index(line,':');
				*(colon) = '\0';
				strcpy(single_ip,line);
				line = colon+1;
				port = atoi(line);
				
				//add me to list -- PUT LIST CODE HERE, add to ComList
                pthread_mutex_lock(&serv_list_mutex);
				if((tmphead = prepend_server_to_list(single_ip,
                    (uint16_t)port,serv_list)) != NULL) {

					serv_list = tmphead;
			    }
                pthread_mutex_unlock(&serv_list_mutex);
                pthread_mutex_lock(&serv_list_len_mutex);
                serv_list_len++;
                pthread_mutex_unlock(&serv_list_len_mutex);

				line = ip+1;		
			}	
		}
    }
}

void print_list(struct serv_list_node *head) {
    struct serv_list_node *p_cur_node;

    printf("List Data (%p):\n", head);
    p_cur_node = head;
    while (p_cur_node) {
        printf("\t%s:%d\n", p_cur_node->ip_addr, p_cur_node->port);
        p_cur_node = p_cur_node->next;
    }
    printf("End List Data\n");
}

int bcast_msg(int orig_sd, gnut_msg_t *p_msg) {
    struct conn_list_node *cur_node;
    int r;

    if (p_msg->header.ttl == 0)
        return 0;

    fprintf(stderr, "-%d-about to lock conn_list_mutex.\n", orig_sd);
    pthread_mutex_lock(&conn_list_mutex);
    fprintf(stderr, "-%d-locked conn_list_mutex.\n", orig_sd);
    cur_node = conn_list;
    fprintf(stderr, "-%d-set cur_node to conn_list.\n", orig_sd);
    while (cur_node) {
        fprintf(stderr, "-%d iteration for sd = %d.\n", orig_sd, cur_node->sd);
        if (cur_node->sd != orig_sd) {
            /* send the message on */
            fprintf(stderr, "-%d--(%d)--attempting to send message.\n",
                orig_sd, cur_node->sd);
            fprintf(stderr, "-%d-(%d)about to lock cur_node->sd_mutex.\n",
                orig_sd, cur_node->sd);
            pthread_mutex_lock(&cur_node->sd_mutex);
            fprintf(stderr, "-%d-(%d)locked cur_node->sd_mutex.\n", orig_sd,
                cur_node->sd);
            r = gnut_send_msg(p_msg, cur_node->sd);
            fprintf(stderr, "-%d-(%d) sent msg with gnut_send_msg [%d].\n", orig_sd, cur_node->sd, r);
            if (r != 0) {
                pthread_mutex_unlock(&cur_node->sd_mutex);
                fprintf(stderr, "-%d-(%d)unlocked cur_node->sd_mutex.\n", orig_sd, cur_node->sd);
                pthread_mutex_unlock(&conn_list_mutex);
                fprintf(stderr, "-%d-(%d)unlocked conn_list_mutex.\n", orig_sd, cur_node->sd);
                return -1;
            }
            pthread_mutex_unlock(&cur_node->sd_mutex);
            fprintf(stderr, "-%d-(%d)unlocked cur_node->sd_mutex.\n", orig_sd, cur_node->sd);
            printf("-%d----sent the message.\n", orig_sd);
        }
        cur_node = cur_node->next;
    }
    fprintf(stderr, "-%d-about to unlock conn_list_mutex.\n", orig_sd);
    pthread_mutex_unlock(&conn_list_mutex);
    fprintf(stderr, "-%d-unlocked conn_list_mutex.\n", orig_sd);
    fprintf(stderr, "-%d-about to return.\n", orig_sd);

    return 0;
}

int fwrd_qhit_msg(gnut_msg_t *p_msg) {
    struct conn_list_node *cur_node;
    int i;

    if (p_msg->header.ttl == 0)
        return 0;

    pthread_mutex_lock(&conn_list_mutex);
    cur_node = conn_list;
    while (cur_node) {
        for (i = 0; i < MAX_IDS; i++) {
            if (memcmp(p_msg->header.message_id, cur_node->msg_id_list[i],
                       GNUT_MSG_ID_LEN) == 0) {
                wang_qhit_msg(p_msg);
                pthread_mutex_lock(&cur_node->sd_mutex);
                gnut_send_msg(p_msg, cur_node->sd);
                pthread_mutex_unlock(&cur_node->sd_mutex);
                pthread_mutex_unlock(&conn_list_mutex);
                return 0;
            }
        }
        cur_node = cur_node->next;
    }
    pthread_mutex_unlock(&conn_list_mutex);

    return 0;
}

int rem_conn_from_list(int sd) {

    struct conn_list_node *cur_node;
    struct conn_list_node *prev_node;

    pthread_mutex_lock(&conn_list_mutex);

    /* here I would remove the conn from the list that matches the
     * socket descriptor passed in. */
    cur_node = conn_list;
    prev_node = conn_list;
    while (cur_node) {
        if (cur_node->sd == sd) {
            if (cur_node == prev_node) { /* are first node in list */
                conn_list = cur_node->next;
                close(cur_node->sd);
                free(cur_node);
            } else { /* not first node in list */
                prev_node->next = cur_node->next;
                close(cur_node->sd);
                free(cur_node);
            }
            pthread_mutex_lock(&conns_cnt_mutex);
            conns_cnt--;
            pthread_cond_signal(&conns_cnt_cond);
            pthread_mutex_unlock(&conns_cnt_mutex);
            break;
        } else {
            prev_node = cur_node;
            cur_node = cur_node->next;
        }
	printf("rem_conn_from_list(sd: %d) inside\n",sd);
    }
    pthread_mutex_unlock(&conn_list_mutex);
	printf("rem_conn_from_list(sd: %d) outside\n",sd);

    return 0;
}

int add_conn_to_list(int sd, uint16_t port, char *ip_addr) {
    struct conn_list_node *cur_list;
    struct conn_list_node *prev_node;
    struct conn_list_node *new_node;

    //lock mutex on conn list
    pthread_mutex_lock(&conn_list_mutex);

    //append new connection
    cur_list = conn_list;
    prev_node = conn_list;
    while(cur_list) {
        prev_node = cur_list;
        cur_list = cur_list->next;
    }
    
    new_node = (struct conn_list_node *)malloc(sizeof(struct conn_list_node));
    if (new_node == NULL) {
        pthread_mutex_unlock(&conn_list_mutex);
        return -1;
    }
    new_node->sd = sd;
    new_node->port = port;
    strncpy(new_node->ip_addr,ip_addr,16);
    new_node->ip_addr[15] = '\0';
    pthread_mutex_init(&new_node->sd_mutex, NULL);
    pthread_mutex_init(&new_node->msg_id_list_mutex, NULL);
    new_node->next = NULL;

    if ((prev_node == cur_list) && (conn_list == NULL)) {
        conn_list = new_node;
    } else {
        prev_node->next = new_node;
    }

    //lock conn_cnt_mutex
    pthread_mutex_lock(&conns_cnt_mutex);
    conns_cnt = conns_cnt + 1;
    //unlock conn_cnt_mutex
    pthread_mutex_unlock(&conns_cnt_mutex);
    //unlock mutex on conn_list
    pthread_mutex_unlock(&conn_list_mutex);

    return 0;
}

void print_conn_list(void) {
    struct conn_list_node *p_cur_node;

    pthread_mutex_lock(&conn_list_mutex);
    printf("Central Conn List Data (%p):\n", conn_list);
    p_cur_node = conn_list;
    while (p_cur_node) {
        printf("\t%s:%d\n", p_cur_node->ip_addr, p_cur_node->port);
        p_cur_node = p_cur_node->next;
    }
    printf("End Central Conn List Data\n");
    pthread_mutex_unlock(&conn_list_mutex);
}

int get_len_of_serv_list(void) {
    struct serv_list_node *p_cur_node;
    int count;

    count = 0;

    p_cur_node = serv_list;
    while (p_cur_node) {
        count++;
        p_cur_node = p_cur_node->next;
    }
    return count;
}

int prebuild_serv_list(char *filename) {
	FILE    *fp;
	char buf[30];
	char *colon;
	char ip[16];
	int port;
    struct serv_list_node *tmphead;

	if((fp = fopen(filename,"r")) == NULL) {
		printf("fopen error\n");
        return -1;
	}

	while(fgets(buf,29,fp)) {
		buf[strlen(buf)] = '\0';

		colon = index(buf,':');
		*(colon) = '\0';
		strcpy(ip,buf);
		colon = colon+1;
		port = atoi(colon);

        pthread_mutex_lock(&serv_list_mutex);
		if((tmphead = append_server_to_list(ip,(uint16_t)port,serv_list)) != NULL) {
			serv_list = tmphead;
		}
        pthread_mutex_unlock(&serv_list_mutex);
        pthread_mutex_lock(&serv_list_len_mutex);
        serv_list_len++;
        pthread_mutex_unlock(&serv_list_len_mutex);

		memset(ip,0x00,16);
	}

    return 0;
}

struct conn_list_node *get_conn_node(int sd) {
    struct conn_list_node *p_cur_node;

    pthread_mutex_lock(&conn_list_mutex);
    p_cur_node = conn_list;
    while (p_cur_node) {
        if (p_cur_node->sd == sd) {
            pthread_mutex_unlock(&conn_list_mutex);
            return p_cur_node;
        }
        p_cur_node = p_cur_node->next;
    }
    pthread_mutex_unlock(&conn_list_mutex);

    return NULL;
}

int wang_qhit_msg(gnut_msg_t *p_msg) {

    // 207.210.233.110:5600
    p_msg->payload.query_hit.port_num = 5600;
    inet_pton(AF_INET, "207.210.233.110",
        (void *)&p_msg->payload.query_hit.ip_addr);

    return 0;
}
