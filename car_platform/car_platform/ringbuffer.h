#ifndef RINGBUFFER_H
#define RINGBUFFER_H

struct ringbuffer
{
    unsigned char *buf;
    int size;
    int head;
    int tail;
    int len;
};

int ringbuf_empty(struct ringbuffer *ringbuf);

int ringbuf_full(struct ringbuffer *ringbuf);

int ringbuf_read(struct ringbuffer *ringbuf, unsigned char *buf, int len);

int ringbuf_write(struct ringbuffer *ringbuf, unsigned char *buf, int len);

int ringbuf_datalen(struct ringbuffer *ringbuf);

int ringbuf_space(struct ringbuffer *ringbuf);

int ringbuf_size(struct ringbuffer *ringbuf);

int ringbuf_reset(struct ringbuffer *ringbuf);

int ringbuf_init(struct ringbuffer *ringbuf, int size);

void ringbuf_deinit(struct ringbuffer *ringbuf);


#endif // RINGBUFFER_H
