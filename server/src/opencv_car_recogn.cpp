#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <opencv2/opencv.hpp>
#include "opencv_car_recogn.h"
#include "image_convert.h"
#include "mainwindow.h"


/* C++ include C */
#ifdef __cplusplus
extern "C" {
#endif
#include "capture.h"
#ifdef __cplusplus
}
#endif

using namespace std;
using namespace cv;


class car_license_recogn    car_recogn_unit;

bool verifySizes_closeImg(const RotatedRect & candidate)
{
	float error = 0.4;
	const float aspect = 44/14; //�����
	int min = 20*aspect*20; //��С����
	int max = 180*aspect*180;  //�������
	float rmin = aspect - aspect*error; //�����������С�����
	float rmax = aspect + aspect*error; //�����������󳤿��

	int area = candidate.size.height * candidate.size.width;
	float r = (float)candidate.size.width/(float)candidate.size.height;
	if(r <1)
		r = 1/r;

	if( (area < min || area > max) || (r< rmin || r > rmax)  )
		return false;
	else
		return true;
}

void RgbConvToGray(const Mat& inputImage,Mat & outpuImage)  //g = 0.3R+0.59G+0.11B
{
	outpuImage = Mat(inputImage.rows ,inputImage.cols ,CV_8UC1);  

	for (int i = 0 ;i<inputImage.rows ;++ i)
	{
		uchar *ptrGray = outpuImage.ptr<uchar>(i); 
		const Vec3b * ptrRgb = inputImage.ptr<Vec3b>(i);
		for (int j = 0 ;j<inputImage.cols ;++ j)
		{
			ptrGray[j] = 0.3*ptrRgb[j][2]+0.59*ptrRgb[j][1]+0.11*ptrRgb[j][0];	
		}
	}
}

void normalPosArea(Mat &intputImg, vector<RotatedRect> &rects_optimal, vector <Mat>& output_area )
{
	float r,angle;
	
	for (int i = 0 ;i< rects_optimal.size() ; ++i)
	{
		printf("%s: %d\n", __FUNCTION__, __LINE__);
		//��ת����
		angle = rects_optimal[i].angle;
		r = (float)rects_optimal[i].size.width / (float) (float)rects_optimal[i].size.height;
		if(r<1)
			angle = 90 + angle;//��תͼ��ʹ��õ������ڸ߶�ͼ��

		Mat rotmat = getRotationMatrix2D(rects_optimal[i].center , angle,1);//��ñ��ξ������
		printf("%s: %d\n", __FUNCTION__, __LINE__);

		Mat img_rotated;
		warpAffine(intputImg ,img_rotated, rotmat, intputImg.size(),INTER_CUBIC);
		if(img_rotated.empty())
		{
			printf("%s: img_rotated is empty\n", __FUNCTION__);
			return ;
		}
		printf("img_rotated.cols: %d, rows: %d\n", img_rotated.cols, img_rotated.rows);
		//imshow("img_rotated",img_rotated);
		//waitKey();
		//�ü�ͼ��
		Size rect_size = rects_optimal[i].size;
		if(r<1)
			swap(rect_size.width, rect_size.height); //�����ߺͿ�
		printf("%s: %d\n", __FUNCTION__, __LINE__);

		Mat rotated_rgb;
		Mat  img_crop;
		if(img_rotated.channels() == 4)
		{
			cvtColor(img_rotated, rotated_rgb, CV_RGBA2RGB);
			printf("### channel: %d\n", img_rotated.channels());
			getRectSubPix(rotated_rgb, rect_size, rects_optimal[i].center , img_crop );
		}
		else
		{
			getRectSubPix(img_rotated, rect_size, rects_optimal[i].center , img_crop );
		}
		printf("%s: %d\n", __FUNCTION__, __LINE__);

		//�ù���ֱ��ͼ�������вü��õ���ͼ��ʹ������ͬ��Ⱥ͸߶ȣ�������ѵ���ͷ���
		Mat resultResized;
		resultResized.create(33,144,CV_8UC3);
		resize(img_crop , resultResized,resultResized.size() , 0,0,INTER_CUBIC);
		printf("%s: %d\n", __FUNCTION__, __LINE__);

		Mat grayResult;
		RgbConvToGray(resultResized ,grayResult);
		//blur(grayResult ,grayResult,Size(3,3));
		equalizeHist(grayResult,grayResult);
		printf("%s: %d\n", __FUNCTION__, __LINE__);

		output_area.push_back(grayResult);
	}
}


car_license_recogn::car_license_recogn(void)
{
	printf("%s: enter ++\n", __FUNCTION__);
}

int car_license_recogn::car_recogn_init(void)
{
	printf("%s: enter ++\n", __FUNCTION__);

	return 0;
}

int car_license_recogn::car_plate_detect(Mat& image)
{
	Mat hsvImg;
    vector <Mat> hsvSplit;

	if(image.empty())
	{
		printf("%s: image is empty!\n", __FUNCTION__);
		return -1;
	}

	cvtColor(image, hsvImg, COLOR_BGR2HSV);
    split(hsvImg,hsvSplit);
	equalizeHist(hsvSplit[2],hsvSplit[2]);
	merge(hsvSplit,hsvImg);

	const int min_blue =100;
	const int max_blue =140;
	int avg_h = (min_blue+max_blue)/2;
	int channels = hsvImg.channels();
	int nRows = hsvImg.rows;
	//ͼ����������Ҫ����ͨ������Ӱ��
	int nCols = hsvImg.cols * channels;

	if (hsvImg.isContinuous())//�����洢�����ݣ���һ�д���
	{
		nCols *= nRows;
		nRows = 1;
	}

	int i, j;
	uchar* p;

	const float  minref_sv = 64; //�ο���S V��ֵ
	const float max_sv = 255; // S V �����ֵ

	for (i = 0; i < nRows; ++i)
	{
		p = hsvImg.ptr<uchar>(i);
		for (j = 0; j < nCols; j += 3)
		{
			int H = int(p[j]); //0-180
			int S = int(p[j + 1]);  //0-255
			int V = int(p[j + 2]);  //0-255
			bool colorMatched = false;

			if (H > min_blue && H < max_blue)
			{
				int Hdiff = 0;
				float Hdiff_p = float(Hdiff) / 40;
				float min_sv = 0;

				if (H > avg_h)
				{
					Hdiff = H - avg_h;
				}	
				else
				{
					Hdiff = avg_h - H;
				}
				min_sv = minref_sv - minref_sv / 2 * (1 - Hdiff_p);
				if ((S > 70&& S < 255) &&(V > 70 && V < 255))
				colorMatched = true;
			}

			if (colorMatched == true) 
			{
				p[j] = 0; p[j + 1] = 0; p[j + 2] = 255;
			}
			else 
			{
				p[j] = 0; p[j + 1] = 0; p[j + 2] = 0;
			}
		}
	}

	Mat src_grey;
	Mat img_threshold;
	vector<Mat> hsvSplit_done;
	split(hsvImg, hsvSplit_done);
	src_grey = hsvSplit_done[2];
	vector <RotatedRect>  rects;
	Mat element = getStructuringElement(MORPH_RECT ,Size(17 ,3));  //����̬ѧ�ĽṹԪ��
	morphologyEx(src_grey ,img_threshold,MORPH_CLOSE,element); 
	morphologyEx(img_threshold,img_threshold,MORPH_OPEN,element);//��̬ѧ����

	vector< vector <Point> > contours;//Ѱ�ҳ������������
	findContours(img_threshold ,contours,RETR_EXTERNAL, CHAIN_APPROX_NONE);//ֻ���������
	//�Ժ�ѡ���������н�һ��ɸѡ
	vector< vector <Point> > ::iterator itc = contours.begin();
	while( itc != contours.end())
	{
		RotatedRect mr = minAreaRect(Mat( *itc )); //����ÿ����������С�н��������
		if(!verifySizes_closeImg(mr))  //�жϾ��������Ƿ����Ҫ��
		{
			itc = contours.erase(itc);
		}
		else     
		{

			rects.push_back(mr);
			++itc;
		}      
	}

	if(rects.empty())
	{
		printf("%s: rects is empty!\n", __FUNCTION__);
		return -1;
	}
	printf("%s: rects size %d\n", __FUNCTION__, rects.size());

	vector <Mat> output_area;
	normalPosArea(image ,rects, output_area);  //���144*33�ĺ�ѡ��������output_area
	printf("%s: %d\n", __FUNCTION__, __LINE__);

	if(output_area.empty())
	{
		printf("%s: output_area is empty!\n", __FUNCTION__);
		return -1;
	}
	printf("%s: %d\n", __FUNCTION__, __LINE__);

	plate_mat = output_area[0];
	imshow("plate mat", plate_mat);

	return 0;
}

int car_license_recogn::car_plate_recogn(void)
{
	printf("%s: enter ++\n", __FUNCTION__);

	return 0;
}

void *opencv_car_recogn_thread(void *arg)
{
	class car_license_recogn *car_recogn = &car_recogn_unit;
	QImage tmpQImage;
	Mat captureMat;
	unsigned char *frame_buf = NULL;
	int frame_len = 0;
	int ret = 0;

    printf("### %s enter ++\n", __FUNCTION__);

	ret = car_recogn->car_recogn_init();
	if(ret != 0)
	{
		printf("%s: car_recogn_init failed !\n", __FUNCTION__);
		return NULL;
	}

    while(1)
    {

		ret = capture_get_framebuf(&frame_buf, &frame_len);
		if(ret != 0)
		{
			usleep(100 *1000);
			continue;
		}
		//printf("frame_buf: %p, frame_len: %d\n", frame_buf, frame_len);

		/* ��v4l2��ȡ����MJPEGͼ��ת��ΪQImage��ʽ */
		tmpQImage = jpeg_to_QImage(frame_buf, frame_len);
		capture_put_framebuf();
		if(tmpQImage.isNull())
		{
			printf("%s ERROR: qImage is null !\n", __FUNCTION__);
			continue;
		}

		//tmpQImage.load("1.jpg"); 

		/* convert qimage to cvMat */
		captureMat = QImage_to_cvMat(tmpQImage).clone();
		if(captureMat.empty())
		{
			printf("%s ERROR: mat is empty\n", __FUNCTION__);
			continue;
		}

		imwrite("error.jpg", captureMat);
		captureMat = imread("error.jpg");
		//imshow("img_input2", captureMat);
		//imwrite("error.jpg", captureMat);

		//waitKey();
#if 1
		ret = car_recogn->car_plate_detect(captureMat);
		if(ret != 0)
		{
			usleep(100 *1000);
			continue;
		}

		tmpQImage = cvMat_to_QImage(car_recogn->plate_mat);
		if(tmpQImage.isNull())
		{
			printf("%s ERROR: plate QImage is null !\n", __FUNCTION__);
			continue;
		}

		mainwin_set_plateImg(tmpQImage);

		ret = car_recogn->car_plate_recogn();
#endif
        sleep(1);
		//waitKey();
    }

	return NULL;
}

int start_car_recogn_task(void)
{
	pthread_t tid;
    int ret;

	ret = pthread_create(&tid, NULL, opencv_car_recogn_thread, NULL);
	if(ret != 0)
	{
		return -1;
	}

    return 0;
}

//using namespace cv;


