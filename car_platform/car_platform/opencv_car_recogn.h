#ifndef _OPENCV_CAR_RECOGN_H_
#define _OPENCV_CAR_RECOGN_H_

#include <opencv2/opencv.hpp>
#include "opencv2/core.hpp"
#include "opencv2/objdetect.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/highgui.hpp"

using namespace std;
using namespace cv;

#define FILE_ANN_XML "../car_platform/ann_xml.xml"
#define FILE_ANN_CHAR_XML "../car_platform/chinese_character.xml"

class car_license_recogn
{
public:
	car_license_recogn(void);
	//~car_license_recogn(void);
	int car_recogn_init(void);
	int car_plate_detect(Mat& image);
	int car_plate_recogn(void);

    unsigned char 	*image_buf;
    unsigned int 	buf_size;

public:
	Mat plate_mat;
};


int start_car_recogn_task(void);


#endif	// _OPENCV_CAR_RECOGN_H_
