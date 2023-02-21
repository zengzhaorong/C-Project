#ifndef CAPTURE_H
#define CAPTURE_H

#include "config.h"
#include <linux/videodev2.h>


#define QUE_BUF_MAX_NUM		5
#define FRAME_BUF_SIZE		(DEFAULT_CAPTURE_WIDTH *DEFAULT_CAPTURE_HEIGH *3)

struct buffer_info
{
    unsigned char *addr;
    int len;
};


struct v4l2cap_info
{
    int run;
    int fd;
    struct v4l2_format format;
    struct buffer_info buffer[QUE_BUF_MAX_NUM];
};


int start_capture_task(void);
int capture_task_stop(void);

int capture_get_framebuf(unsigned char **buf_ptr, int *len);
void capture_put_framebuf(void);
int v4l2cap_update_newframe(unsigned char *data, int len);
int capture_get_newframe(unsigned char *data, int size, int *len);
int v4l2cap_clear_newframe(void);
int newframe_mem_init(void);


#endif // CAPTURE_H
