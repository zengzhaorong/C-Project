#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/socket.h>
#include "socket_server.h"
#include "ringbuffer.h"
#include <errno.h>
#include "mainwindow.h"

static struct serverInfo server_info;

struct videoframe_info videoframe_buf;

/* update frame data for video buffer - 为视频缓存更新图像数据 */
int videoframe_update(uint8_t *data, int len)
{
    int flush_len = 0;

    if(len > FRAME_BUF_SIZE)
        flush_len = FRAME_BUF_SIZE;
    else
        flush_len = len;

    pthread_mutex_lock(&videoframe_buf.mutex);
    memset(videoframe_buf.frame_buf, 0, FRAME_BUF_SIZE);
    memcpy(videoframe_buf.frame_buf, data, flush_len);
    videoframe_buf.frame_len = flush_len;
    pthread_mutex_unlock(&videoframe_buf.mutex);

    return 0;
}

/* get one video frame - 从缓存中获取一帧图像数据 */
int videoframe_get_one(uint8_t *data, uint32_t size, int *len)
{
    uint32_t tmpLen;

    if(data==NULL || size<=0)
    {
        return -1;
    }

    tmpLen = (videoframe_buf.frame_len <size ? videoframe_buf.frame_len:size);
    if(tmpLen < videoframe_buf.frame_len)
    {
        printf("Warning: %s: bufout size[%d] < frame size[%d] !!!\n", __FUNCTION__, size, videoframe_buf.frame_len);
    }
    if(tmpLen <= 0)
    {
        return -1;
    }

    pthread_mutex_lock(&videoframe_buf.mutex);
    memcpy(data, videoframe_buf.frame_buf, tmpLen);
    pthread_mutex_unlock(&videoframe_buf.mutex);
    *len = tmpLen;

    return 0;
}


/* clear frame data - 清除图像缓存数据 */
int videoframe_clear(void)
{
    pthread_mutex_lock(&videoframe_buf.mutex);
    memset(videoframe_buf.frame_buf, 0, videoframe_buf.frame_len);
    pthread_mutex_unlock(&videoframe_buf.mutex);
    videoframe_buf.frame_len = 0;

    return 0;
}

/* initial frame memory to sotre the newest received frame - 初始化一片缓存用来存放最新接收到的图像数据 */
int videoframe_mem_init(void)
{
    uint8_t *buf;

    buf = (uint8_t *)calloc(1, FRAME_BUF_SIZE);
    if(buf == NULL)
    {
        printf("ERROR: %s: malloc failed\n", __FUNCTION__);
        return -1;
    }
    videoframe_buf.frame_len = 0;
    videoframe_buf.frame_buf = buf;
    pthread_mutex_init(&videoframe_buf.mutex, NULL);

    return 0;
}


int server_0x01_login(struct clientInfo *client, uint8_t *data, int len, uint8_t *ack_data, int size, int *ack_len)
{
    uint8_t usr_name[32] = {0};
    uint8_t passwd[32] = {0};
    int tmplen = 0;
    int ret = 0;

    /* user name */
    memcpy(usr_name, data +tmplen, 32);
    tmplen += 32;

    /* password */
    memcpy(passwd, data +tmplen, 32);
    tmplen += 32;

    /* ack part */
    tmplen = 0;
    memcpy(ack_data, &ret, 4);
    tmplen += 4;

    *ack_len = tmplen;

    return 0;
}

int server_0x03_heartbeat(struct clientInfo *client, uint8_t *data, int len, uint8_t *ack_data, int size, int *ack_len)
{
    uint32_t tmpTime;
    int tmplen = 0;
    int ret;

    /* request part */
    memcpy(&tmpTime, data, 4);
    //printf("%s: time: %ld\n", __FUNCTION__, tmpTime);

    /* ack part */
    ret = 0;
    memcpy(ack_data +tmplen, &ret, 4);
    tmplen += 4;

    tmpTime = (uint32_t)time(NULL);
    memcpy(ack_data +tmplen, &tmpTime, 4);
    tmplen += 4;

    *ack_len = tmplen;

    return 0;
}

int client_0x10_sendCaptureFrame(struct clientInfo *client, uint8_t *data, int len, uint8_t *ack_data, int size, int *ack_len)
{
    int format = 0;
    int frame_len = 0;
    int offset = 0;

    /* format */
    memcpy(&format, data +offset, 4);
    offset += 4;

    /* frame len */
    memcpy(&frame_len, data +offset, 4);
    offset += 4;

    /* update frame data */
    videoframe_update(data +offset, frame_len);

    return 0;
}

int server_init(struct serverInfo *server, int port)
{
    int ret;

    memset(server, 0, sizeof(struct serverInfo));

    server->fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(server->fd < 0)
    {
        ret = -1;
        goto ERR_0;
    }

    server->srv_addr.sin_family = AF_INET;
    server->srv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server->srv_addr.sin_port = htons(port);

    ret = bind(server->fd, (struct sockaddr *)&server->srv_addr, sizeof(server->srv_addr));
    if(ret != 0)
    {
        ret = -2;
        goto ERR_1;
    }

    ret = listen(server->fd, MAX_LISTEN_NUM);
    if(ret != 0)
    {
        ret = -3;
        goto ERR_2;
    }

    proto_init();

    return 0;

ERR_2:

ERR_1:
    close(server->fd);

ERR_0:

    return ret;
}

struct clientInfo *socket_set_client(int fd, struct sockaddr_in *cli_addr)
{
    struct serverInfo *server = &server_info;
    int client_index;
    int i;

    for(i=0; i<MAX_CLIENT_NUM; i++)
    {
        if(server->client_used[i] == 0)		// can use this
        {
            client_index = i;
            break;
        }
    }

    if(i >= MAX_CLIENT_NUM)
    {
        printf("%s: ERROR: Not find idle client!\n", __FUNCTION__);
        return NULL;
    }

    server->client[client_index].fd = fd;
    memcpy(&server->client[client_index].addr, cli_addr, sizeof(struct sockaddr_in));
    server->client_used[client_index] = 1;
    server->client_cnt ++;

    printf("%s: set client[%d] fd=%d, client_cnt=%d\n", __FUNCTION__, client_index, fd, server->client_cnt);
    return &server->client[client_index];
}

int server_sendData(void *arg, uint8_t *data, int len)
{
    struct clientInfo *client = (struct clientInfo *)arg;
    int total = 0;
    int ret;

    if(client->state==STATE_CLOSE || data==NULL)
        return -1;

    // lock
    pthread_mutex_lock(&client->send_mutex);
    do{
        ret = send(client->fd, data +total, len -total, MSG_NOSIGNAL);
        if(ret < 0)
        {
            if(errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN)
            {
                usleep(1000);
                continue;
            }
            else
            {
                client->state = STATE_CLOSE;
                perror("socket send failed");
                printf("ret: %d, errno = %d\n", ret, errno);
                break;
            }
        }
        total += ret;
    }while(total < len);
    // unlock
    pthread_mutex_unlock(&client->send_mutex);

    return total;
}

int server_recvData(struct clientInfo *client)
{
    uint8_t *tmpBuf = client->tmpBuf;
    int len, space;
    int ret = 0;

    if(client==NULL)
        return -1;

    if(client->state == STATE_CLOSE)
        return -1;

    space = ringbuf_space(&client->recvRingBuf);

    memset(tmpBuf, 0, PROTO_PACK_MAX_LEN);
    len = recv(client->fd, tmpBuf, PROTO_PACK_MAX_LEN>space ? space:PROTO_PACK_MAX_LEN, 0);
    if(len > 0)
    {
        ret = ringbuf_write(&client->recvRingBuf, tmpBuf, len);
    }
    else if(len < 0)
    {
        if(errno != EINTR && errno != EWOULDBLOCK && errno != EAGAIN)
        {
            client->state = STATE_CLOSE;
            perror("socket recv failed");
            printf("ret: %d, errno = %d\n", len, errno);
        }
    }

    return ret;
}

void socket_reset_client(struct clientInfo *client)
{
    struct serverInfo *server = &server_info;
    int client_index = 0;
    int i;

    for(i=0; i<MAX_CLIENT_NUM; i++)
    {
        if(server->client[i].fd == client->fd)		// can use this
        {
            client_index = i;
            break;
        }
    }

    server->client_used[client_index] = 0;
    server->client_cnt --;

    close(client->fd);

    printf("%s: reset client[%d] fd=%d\n", __FUNCTION__, client_index, client->fd);
}

int server_protoAnaly(struct clientInfo *client, uint8_t *pack, uint32_t pack_len)
{
    uint8_t *ack_buf = client->ack_buf;
    uint8_t *tmpBuf = client->tmpBuf;
    uint8_t seq = 0, cmd = 0;
    int data_len = 0;
    uint8_t *data = NULL;
    int ack_len = 0;
    int tmpLen = 0;
    int ret;

    if(pack==NULL || pack_len<=0)
        return -1;

    ret = proto_analyPacket(pack, pack_len, &seq, &cmd, &data_len, &data);
    if(ret != 0)
        return -1;
    //printf("%s: cmd: 0x%02x, seq: %d, pack_len: %d, data_len: %d\n", __FUNCTION__, cmd, seq, pack_len, data_len);

    switch(cmd)
    {
        case 0x01:
            ret = server_0x01_login(client, data, data_len, ack_buf, PROTO_PACK_MAX_LEN, &ack_len);
            break;

        case 0x02:
            break;

        case 0x03:
            ret = server_0x03_heartbeat(client, data, data_len, ack_buf, PROTO_PACK_MAX_LEN, &ack_len);
            break;

        case 0x10:
            ret = client_0x10_sendCaptureFrame(client, data, data_len, ack_buf, PROTO_PACK_MAX_LEN, &ack_len);
            break;

        default:
            printf("ERROR: protocol cmd[0x%02x] not exist!\n", cmd);
            break;
    }

    /* send ack data */
    if(ret==0 && ack_len>0)
    {
        proto_makeupPacket(seq, cmd, ack_len, ack_buf, tmpBuf, PROTO_PACK_MAX_LEN, &tmpLen);
        server_sendData(client, tmpBuf, tmpLen);
    }

    return 0;
}

int server_protoHandle(struct clientInfo *client)
{
    int recv_ret;
    int det_ret;

    if(client == NULL)
        return -1;

    recv_ret = server_recvData(client);
    det_ret = proto_detectPack(&client->recvRingBuf, &client->detectInfo, client->packBuf, \
                            sizeof(client->packBuf), &client->packLen);
    if(det_ret == 0)
    {
        server_protoAnaly(client, client->packBuf, client->packLen);
    }

    if(recv_ret<=0 && det_ret!=0)
    {
        usleep(30*1000);
    }

    return 0;
}

void *socket_handle_thread(void *arg)
{
    struct clientInfo *client = (struct clientInfo *)arg;
    int flags = 0;
    int ret;

    printf("%s %d: enter ++\n", __FUNCTION__, __LINE__);

    pthread_mutex_init(&client->send_mutex, NULL);

    flags = fcntl(client->fd, F_GETFL, 0);
    fcntl(client->fd, F_SETFL, flags | O_NONBLOCK);

    ret = ringbuf_init(&client->recvRingBuf, SVR_RECVBUF_SIZE);
    if(ret < 0)
        return NULL;

    client->protoHandle = proto_register(client, server_sendData, SVR_SENDBUF_SIZE);
    client->state = STATE_CONNECTED;

    while(client->state != STATE_CLOSE)
    {
        server_protoHandle(client);
    }

    proto_unregister(client->protoHandle);
    ringbuf_deinit(&client->recvRingBuf);

    socket_reset_client(client);
    printf("%s: exit --\n", __FUNCTION__);
    return NULL;
}

void *socket_listen_thread(void *arg)
{
    struct serverInfo *server = &server_info;
    struct sockaddr_in cli_addr;
    struct clientInfo *client;
    pthread_t tid;
    int cli_sockfd;
    int tmpLen;
    int ret;

    (void)arg;

    ret = server_init(server, DEFAULT_SERVER_PORT);
    if(ret != 0)
    {
        return NULL;
    }

    while(1)
    {
        if(server->client_cnt >= MAX_CLIENT_NUM)
        {
            sleep(3);
            continue;
        }

        memset(&cli_addr, 0, sizeof(struct sockaddr_in));
        cli_sockfd = accept(server->fd, (struct sockaddr *)&cli_addr, (socklen_t *)&tmpLen);
        if(cli_sockfd < 0)
            continue;
        printf("%s %d: *************** accept socket success, sock fd: %d ...\n", __FUNCTION__, __LINE__, cli_sockfd);

        client = socket_set_client(cli_sockfd, &cli_addr);
        if(client == NULL)
        {
            printf("ERROR: %s %d: socket_set_client failed !!!\n", __FUNCTION__, __LINE__);
            continue;
        }

        ret = pthread_create(&tid, NULL, socket_handle_thread, client);
        if(ret != 0)
        {
            printf("ERROR: %s %d: pthread_create failed !!!\n", __FUNCTION__, __LINE__);
        }

    }

    return NULL;
}


int start_socket_server_task(void)
{
    pthread_t tid;
    int ret;

    videoframe_mem_init();

    ret = pthread_create(&tid, NULL, socket_listen_thread, NULL);
    if(ret != 0)
    {
        return -1;
    }

    return 0;
}

