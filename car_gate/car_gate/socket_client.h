#ifndef SOCKET_CLIENT_H
#define SOCKET_CLIENT_H

#include <netinet/in.h>
#include "ringbuffer.h"
#include "protocol.h"


#define CLIENT_NUM_MAX				10
#define CLI_SENDBUF_SIZE			PROTO_PACK_MAX_LEN
#define CLI_RECVBUF_SIZE			(PROTO_PACK_MAX_LEN*3)
#define HEARTBEAT_INTERVAL_S		10



typedef enum
{
    STATE_DISABLE,
    STATE_DISCONNECT,
    STATE_CONNECTED,
    STATE_LOGIN,
}sockState_e;

struct clientInfo
{
    int fd;
    int protoHandle;
    pthread_mutex_t	send_mutex;
    sockState_e state;
    struct sockaddr_in 	srv_addr;		// server ip addr
    struct ringbuffer recvRingBuf;			// socket receive data ring buffer
    struct detect_info detectInfo;
    uint8_t packBuf[PROTO_PACK_MAX_LEN];		// protocol packet data buffer
    int packLen;
    uint8_t ack_buf[PROTO_PACK_MAX_LEN];
    uint8_t tmpBuf[PROTO_PACK_MAX_LEN];
};

int start_socket_client_task(void);



#endif // SOCKET_CLIENT_H
