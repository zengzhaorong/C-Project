#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "protocol.h"
#include "ringbuffer.h"


#define MAX_PROTO_OBJ	3
struct proto_object protoObject[MAX_PROTO_OBJ];
static uint8_t tmp_protoBuf[PROTO_PACK_MAX_LEN];


int proto_makeupPacket(uint8_t seq, uint8_t cmd, int len, uint8_t *data, \
                                uint8_t *outbuf, int size, int *outlen)
{
    uint8_t *packBuf = outbuf;
    int packLen = 0;

    if((len!=0 && data==NULL) || outbuf==NULL || outlen==NULL)
        return -1;

    /* if outbuf is not enough */
    if(PROTO_DATA_OFFSET +len +1 > size)
    {
        printf("ERROR: %s: outbuf size [%d:%d] is not enough !!!\n", __FUNCTION__, size, PROTO_DATA_OFFSET +len +1);
        return -2;
    }

    packBuf[PROTO_HEAD_OFFSET] = PROTO_HEAD;
    packLen += 1;

    memcpy(packBuf +PROTO_VERIFY_OFFSET, PROTO_VERIFY, 4);
    packLen += 4;

    packBuf[PROTO_SEQ_OFFSET] = seq;
    packLen += 1;

    packBuf[PROTO_CMD_OFFSET] = cmd;
    packLen += 1;

    memcpy(packBuf +PROTO_LEN_OFFSET, &len, 4);
    packLen += 4;

    memcpy(packBuf +PROTO_DATA_OFFSET, data, len);
    packLen += len;

    packBuf[PROTO_DATA_OFFSET +len] = PROTO_TAIL;
    packLen += 1;

    *outlen = packLen;

    return 0;
}

int proto_analyPacket(uint8_t *pack, int packLen, uint8_t *seq, \
                                uint8_t *cmd, int *len, uint8_t **data)
{

    if(pack==NULL || seq==NULL || cmd==NULL || len==NULL || data==NULL)
        return -1;

    if(packLen < PROTO_PACK_MIN_LEN)
        return -2;

    *seq = pack[PROTO_SEQ_OFFSET];

    *cmd = pack[PROTO_CMD_OFFSET];

    memcpy(len, pack +PROTO_LEN_OFFSET, 4);

    if(*len +PROTO_PACK_MIN_LEN != packLen)
    {
        return -1;
    }

    if(*len > 0)
        *data = pack + PROTO_DATA_OFFSET;

    return 0;
}

int proto_detectPack(struct ringbuffer *ringbuf, struct detect_info *detect, \
                            uint8_t *proto_data, int size, int *proto_len)
{
    unsigned char buf[256];
    int len;
    char veri_buf[] = PROTO_VERIFY;
    int tmp_protoLen;
    uint8_t byte;

    if(ringbuf==NULL || proto_data==NULL || proto_len==NULL || size<PROTO_PACK_MIN_LEN)
        return -1;

    tmp_protoLen = *proto_len;

    /* get and check protocol head */
    if(!detect->head)
    {
        while(ringbuf_datalen(ringbuf) > 0)
        {
            ringbuf_read(ringbuf, &byte, 1);
            if(byte == PROTO_HEAD)
            {
                proto_data[0] = byte;
                tmp_protoLen = 1;
                detect->head = 1;
                //printf("********* detect head\n");
                break;
            }
        }
    }

    /* get and check verify code */
    if(detect->head && !detect->verify)
    {
        while(ringbuf_datalen(ringbuf) > 0)
        {
            ringbuf_read(ringbuf, &byte, 1);
            if(byte == veri_buf[tmp_protoLen-1])
            {
                proto_data[tmp_protoLen] = byte;
                tmp_protoLen ++;
                if(tmp_protoLen == 1+strlen(PROTO_VERIFY))
                {
                    detect->verify = 1;
                    //printf("********* detect verify\n");
                    break;
                }
            }
            else
            {
                if(byte == PROTO_HEAD)
                {
                    proto_data[0] = byte;
                    tmp_protoLen = 1;
                    detect->head = 1;
                }
                else
                {
                    tmp_protoLen = 0;
                    detect->head = 0;
                }
            }
        }
    }

    /* get other protocol data */
    if(detect->head && detect->verify)
    {
        while(ringbuf_datalen(ringbuf) > 0)
        {
            if(tmp_protoLen < PROTO_DATA_OFFSET)	// read data_len
            {
                len = ringbuf_read(ringbuf, buf, sizeof(buf) < PROTO_DATA_OFFSET -tmp_protoLen ? \
                                                    sizeof(buf) : PROTO_DATA_OFFSET -tmp_protoLen);
                if(len > 0)
                {
                    memcpy(proto_data +tmp_protoLen, buf, len);
                    tmp_protoLen += len;
                }
                if(tmp_protoLen >= PROTO_DATA_OFFSET)
                {
                    memcpy(&len, proto_data +PROTO_LEN_OFFSET, 4);
                    detect->pack_len = PROTO_DATA_OFFSET +len +1;
                    if(detect->pack_len > size)
                    {
                        printf("ERROR: %s: pack len[%d] > buf size[%d]\n", __FUNCTION__, size, detect->pack_len);
                        memset(detect, 0, sizeof(struct detect_info));
                    }
                }
            }
            else	// read data
            {
                len = ringbuf_read(ringbuf, buf, sizeof(buf) < detect->pack_len -tmp_protoLen ? \
                                                    sizeof(buf) : detect->pack_len -tmp_protoLen);
                if(len > 0)
                {
                    memcpy(proto_data +tmp_protoLen, buf, len);
                    tmp_protoLen += len;
                    if(tmp_protoLen == detect->pack_len)
                    {
                        if(proto_data[tmp_protoLen-1] != PROTO_TAIL)
                        {
                            printf("%s : packet data error, no detect tail!\n", __FUNCTION__);
                            memset(detect, 0, sizeof(struct detect_info));
                            tmp_protoLen = 0;
                            break;
                        }
                        *proto_len = tmp_protoLen;
                        memset(detect, 0, sizeof(struct detect_info));
                        //printf("%s : get complete protocol packet, len: %d\n", __FUNCTION__, *proto_len);
                        return 0;
                    }
                }
            }
        }
    }

    *proto_len = tmp_protoLen;

    return -1;
}

/* return: handle */
int proto_register(void *arg, send_func_t send_func, int buf_size)
{
    int handle = -1;
    int i;

    for(i=0; i<MAX_PROTO_OBJ; i++)
    {
        if(protoObject[i].used == 0)
        {
            protoObject[i].arg = arg;
            protoObject[i].send_func = send_func;

            protoObject[i].buf_size = buf_size;
            protoObject[i].send_buf = (uint8_t *)malloc(buf_size);
            if(protoObject[i].send_buf == NULL)
                return -1;

            protoObject[i].used = 1;
            handle = i;
            break;
        }
    }

    return handle;
}

void proto_unregister(int handle)
{
    protoObject[handle].used = 0;

    if(protoObject[handle].send_buf != NULL)
        free(protoObject[handle].send_buf);

}

int proto_init(void)
{
    memset(&protoObject, 0, sizeof(protoObject));

    return 0;
}


