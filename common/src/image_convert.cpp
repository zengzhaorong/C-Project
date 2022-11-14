#include <stdio.h>
#include <string.h>
#include "image_convert.h"

/* C++ include C */
#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif


//using namespace cv;


/* jpge/mjpge convert to QImage */
QImage jpeg_to_QImage(unsigned char *data, int len)
{
	QImage qtImage;

	if(data==NULL || len<=0)
		return qtImage;

	qtImage.loadFromData(data, len);
	if(qtImage.isNull())
	{
		printf("ERROR: %s: QImage is null !\n", __FUNCTION__);
	}

	return qtImage;
}

