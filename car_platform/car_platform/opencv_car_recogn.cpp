#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "opencv2/core.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/highgui.hpp"
#include <opencv2/ml.hpp>
#include "opencv2/ml/ml.hpp"
#include "opencv_car_recogn.h"
#include <opencv2/opencv.hpp>
#include <opencv2/core.hpp>
#include <opencv2/imgproc/types_c.h>
#include "image_convert.h"
#include "socket_server.h"
#include "mainwindow.h"
#include <QTime>
#include <QImage>
#include "sql_db.h"
#include "config.h"

using namespace std;
using namespace cv;
using namespace cv::ml;


class car_license_recogn    car_recogn_unit;
extern MainWindow *mainwindow;


bool verifySizes_closeImg(const RotatedRect & candidate)
{
	float error = 0.4;
	const float aspect = 44/14; //长宽比
	int min = 20*aspect*20; //最小区域
	int max = 180*aspect*180;  //最大区域
	float rmin = aspect - aspect*error; //考虑误差后的最小长宽比
	float rmax = aspect + aspect*error; //考虑误差后的最大长宽比

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
		//旋转区域
		angle = rects_optimal[i].angle;
		r = (float)rects_optimal[i].size.width / (float) (float)rects_optimal[i].size.height;
		if(r<1)
			angle = 90 + angle;//旋转图像使其得到长大于高度图像。

		Mat rotmat = getRotationMatrix2D(rects_optimal[i].center , angle,1);//获得变形矩阵对象

		Mat img_rotated;
		warpAffine(intputImg ,img_rotated, rotmat, intputImg.size(),INTER_CUBIC);
		if(img_rotated.empty())
		{
			printf("%s: img_rotated is empty\n", __FUNCTION__);
			return ;
		}

		//imshow("img_rotated",img_rotated);
		//waitKey();
		//裁剪图像
		Size rect_size = rects_optimal[i].size;
		if(r<1)
			swap(rect_size.width, rect_size.height); //交换高和宽

		Mat rotated_rgb;
		Mat  img_crop;
		if(img_rotated.channels() == 4)
		{
			cvtColor(img_rotated, rotated_rgb, CV_RGBA2RGB);
			getRectSubPix(rotated_rgb, rect_size, rects_optimal[i].center , img_crop );
		}
		else
		{
			getRectSubPix(img_rotated, rect_size, rects_optimal[i].center , img_crop );
		}

		//用光照直方图调整所有裁剪得到的图像，使具有相同宽度和高度，适用于训练和分类
		Mat resultResized;
		resultResized.create(33,144,CV_8UC3);
		resize(img_crop , resultResized,resultResized.size() , 0,0,INTER_CUBIC);

		Mat grayResult;
		RgbConvToGray(resultResized ,grayResult);
		//blur(grayResult ,grayResult,Size(3,3));
		equalizeHist(grayResult,grayResult);

		output_area.push_back(grayResult);
	}
}

Mat projectHistogram(const Mat& img ,int t)  //ˮƽ��ֱֱ��ͼ,0Ϊ����ͳ��
{                                            //1Ϊ����ͳ��
	int sz = (t)? img.rows: img.cols;
	Mat mhist = Mat::zeros(1, sz ,CV_32F);

	for(int j = 0 ;j < sz; j++ )
	{
		Mat data = (t)?img.row(j):img.col(j);
		mhist.at<float>(j) = countNonZero(data);
	}

	double min,max;
	minMaxLoc(mhist , &min ,&max);

	if(max > 0)
		mhist.convertTo(mhist ,-1,1.0f/max , 0);

	return mhist;
}

void features(const Mat & in , Mat & out ,int sizeData)
{
	Mat vhist = projectHistogram(in , 1); //ˮƽֱ��ͼ
	Mat hhist = projectHistogram(in , 0);  //��ֱֱ��ͼ

	Mat lowData;
	resize(in , lowData ,Size(sizeData ,sizeData ));
	int numCols = vhist.cols + hhist.cols + lowData.cols * lowData.cols;
	out = Mat::zeros(1, numCols , CV_32F);

	int j = 0;
	for (int i =0 ;i<vhist.cols ; ++i)
	{
		out.at<float>(j) = vhist.at<float>(i);
		j++;
	}
	for (int i=0 ; i < hhist.cols ;++i)
	{
		out.at<float>(j) = hhist.at<float>(i);
	}
	for(int x =0 ;x<lowData.rows ;++x)
	{
		for (int y =0 ;y < lowData.cols ;++ y)
		{
			out.at<float>(j) = (float)lowData.at<unsigned char>(x,y);
			j++;
		}
	}

}


void char_sort(vector <RotatedRect > & in_char ) //���ַ������������
{
	vector <RotatedRect >  out_char;
	const int length = 7;           //7���ַ�
	int index[length] = {0,1,2,3,4,5,6};
	float centerX[length];

	for (int i=0;i < length ; ++ i)
	{
		centerX[i] = in_char[i].center.x;
	}

	for (int j=0;j <length;j++) {
		for (int i=length-2;i >= j;i--)
			if (centerX[i] > centerX[i+1])
			{
				float t=centerX[i];
				centerX[i]=centerX[i+1];
				centerX[i+1]=t;

				int tt = index[i];
				index[i] = index[i+1];
				index[i+1] = tt;
			}
	}

	for(int i=0;i<length ;i++)
		out_char.push_back(in_char[(index[i])]);

	in_char.clear();     //���in_char
	in_char = out_char; //������õ��ַ������������¸�ֵ��in_char
}

bool char_verifySizes(const RotatedRect & candidate)
{
	float aspect = 45.0f/90.0f;
	float width,height;
	if (candidate.size.width >=candidate.size.height)
	{
		width = (float) candidate.size.height;
		height = (float) candidate.size.width;
	}

	else 
	{
		width = (float) candidate.size.width;
		height  = (float)candidate.size.height;
	}
	//����ȷ�����˸߱ȿ�Ҫ��

	float charAspect = (float) width/ (float)height;//���߱�

	float error = 0.5;
	float minHeight = 15;  //��С�߶�11
	float maxHeight = 33;  //���߶�33

	float minAspect = 0.05;  //���ǵ�����1����С������Ϊ0.15
	float maxAspect = 1.0;

	if( charAspect > minAspect && charAspect <= 1.0
		&&  height>= minHeight && height< maxHeight) //��0����maxAspect�����ȡ��߶�����������
		return true;
	else
		return false;
}
	

bool clearLiuDing(Mat& img)
{
	vector<float> fJump;
	int whiteCount = 0;
	const int x = 7;
	const int x1 =2;
	const int y = 2;
	Mat jump = Mat::zeros(1, img.rows, CV_32F);
	Mat jump_col = Mat::zeros(1,img.cols,CV_32F);
	for (int i = 0; i < img.rows; i++)
	{
		int jumpCount = 0;
		for (int j = 0; j < img.cols - 1; j++)
		{
			if (img.at<char>(i, j) != img.at<char>(i, j + 1)) 
			{
				jumpCount++;
			}   

			if (img.at<uchar>(i, j) == 255) 
			{
				whiteCount++;
			}
		}

		jump.at<float>(i) = (float)jumpCount;
	}
	int iCount = 0;

	for (int i = 0; i < img.rows; i++) 
	{
		fJump.push_back(jump.at<float>(i));
		if (jump.at<float>(i) >= 16 && jump.at<float>(i) <= 45) 
		{
			iCount++;//�����ַ�����һ����������
		}
	}

	////�����Ĳ��ǳ���
	if (iCount * 1.0 / img.rows <= 0.40)
	{
		return false; //�������������������ҲҪ��һ������ֵ��
	}
	//�����㳵�Ƶ�����
	if (whiteCount * 1.0 / (img.rows * img.cols) < 0.15 ||
		whiteCount * 1.0 / (img.rows * img.cols) > 0.50)
	{
		return false;
	}

	for (int i = 0; i < img.rows; i++) 
	{
		if (jump.at<float>(i) <= x||i<2||i>(img.rows-2)) 
		{
			for (int j = 0; j < img.cols; j++)
			{
				img.at<char>(i, j) = 0;
			}
		}

	}

	for (int z = 0;z <img.cols; z++)

	{
		//int x = img.cols
		if ( z<2 ||z>(img.cols-2))
		{
			for (int w=0; w < img.rows;w++)
			{
				img.at<char>(w,z)=0;
			}
			
		}
	}
	//medianBlur(img,img,3);
	return true;
}

int char_segment(const Mat & inputImg,vector <Mat>& dst_mat)//�õ�20*20�ı�׼�ַ��ָ�ͼ��
{
	Mat img_threshold;
	//blur(inputImg,inputImg,Size(7,7));
	threshold(inputImg ,img_threshold , 180,255 ,THRESH_BINARY );
	//Mat element = getStructuringElement(MORPH_RECT ,Size(3 ,3));  //����̬ѧ�ĽṹԪ��
	//morphologyEx(img_threshold ,img_threshold,CV_MOP_CLOSE,element);  //��̬ѧ����

	//imshow ("img_thresho00ld",img_threshold);
	//waitKey();
	Mat img_contours;
	img_threshold.copyTo(img_contours);

    printf("%s: %d ++\n", __FUNCTION__, __LINE__);
    //imshow ("img_threshold",img_threshold);
	if (!clearLiuDing(img_contours))
    {
        printf("%s: %d ++\n", __FUNCTION__, __LINE__);
       std::cout << "不是车牌" << endl;
	   return -1;
	   //waitKey();
	 }
	else
    {
        printf("%s: %d ++\n", __FUNCTION__, __LINE__);
    //imshow("img_cda",img_contours);
	//waitKey();
	Mat result2;
	inputImg.copyTo(result2);
	
	 vector < vector <Point> > contours;
	 findContours(img_contours ,contours,RETR_EXTERNAL,CHAIN_APPROX_NONE);

	 vector< vector <Point> > ::iterator itc = contours.begin();
	 vector<RotatedRect> char_rects;
	  //vector<Mat> char_rects;

	//Mat result2;
   // img_contours.copyTo(result2);
	 
	 drawContours(result2,contours,-1, Scalar(0,255,255), 1); 
	 //imshow("result22",result2);
	 //waitKey();
     printf("%s: %d ++\n", __FUNCTION__, __LINE__);

	 while( itc != contours.end())
   {
		RotatedRect minArea = minAreaRect(Mat( *itc )); //����ÿ����������С�н��������
		Point2f vertices[4];
		minArea.points(vertices);

		if(!char_verifySizes(minArea))  //�жϾ��������Ƿ����Ҫ��
		{
			itc = contours.erase(itc);}
		//contours.

		else     
		{
			++itc; 
			char_rects.push_back(minArea);  	
		}  
    		
	}
     printf("%s: %d ++\n", __FUNCTION__, __LINE__);

	/* while (itc!=contours.end())
	 {  

		   
		 Rect mr= boundingRect(Mat(*itc));  
		 Mat auxRoi(img_threshold, mr);

		 if(char_verifySizes(auxRoi))
		 {  
			 
		 }  

*/
	 //imshow("char1",char_rects[1]);
	 //imshow("char2",char_rects[2]);
	 //imshow("char3",char_rects[3]);
	//imshow("char4",char_rects[4]);
	 ////imshow("char5",char_rects[5]);
	/// imshow("char6",char_rects[6]);
	// imshow("char7",char_rects[0]);



	// waitKey();
    char_sort(char_rects); //���ַ�����

	vector <Mat> char_mat;

    printf("%s: %d ++\n", __FUNCTION__, __LINE__);
    for (int i = 0; i<char_rects.size() ;i++ )
	{
		Rect roi = char_rects[i].boundingRect();
    	if(0 >roi.x || 0 >roi.width || 0 >roi.y || 0 >roi.height || roi.x + roi.width >img_threshold.cols || roi.y + roi.height >img_threshold.rows)
		{
			printf("Error: x=%d, y=%d, w=%d, h=%d\n", roi.x, roi.y, roi.width, roi.height);
			return -1;
		}
		//char_mat.push_back(Mat(inputImg,char_rects[i].boundingRect())) ;
		char_mat.push_back(Mat(img_threshold,char_rects[i].boundingRect())) ;
	}
    printf("%s: %d ++\n", __FUNCTION__, __LINE__);

	//imshow("char_mat1",char_mat[0]);
	//imshow("char_mat2",char_mat[1]);
	//imshow("char_mat3",char_mat[2]);
	//imshow("char_mat4",char_mat[3]);
	//imshow("char_mat5",char_mat[4]);
	//imshow("char_mat6",char_mat[5]);
	//imshow("char_mat7",char_mat[6]);
	//waitKey();
//
	Mat train_mat(2,3,CV_32FC1);
	int length ;
	dst_mat.resize(7);
	Point2f srcTri[3];  
	Point2f dstTri[3];

	for (int i = 0; i==0;i++)
	{
		srcTri[0] = Point2f( 0,0 );  
		srcTri[1] = Point2f( char_mat[i].cols - 1, 0 );  
		srcTri[2] = Point2f( 0, char_mat[i].rows - 1 );
		length = char_mat[i].rows > char_mat[i].cols?char_mat[i].rows:char_mat[i].cols;
		dstTri[0] = Point2f( 0.0, 0.0 );  
		dstTri[1] = Point2f( length, 0.0 );  
		dstTri[2] = Point2f( 0.0, length ); 
		train_mat = getAffineTransform( srcTri, dstTri );
		dst_mat[i]=Mat::zeros(length,length,char_mat[i].type());		
		warpAffine(char_mat[i],dst_mat[i],train_mat,dst_mat[i].size(),INTER_LINEAR,BORDER_CONSTANT,Scalar(0));
		//resize(dst_mat[i],dst_mat[i],Size(20,20),0,0,CV_INTER_CUBIC);  //�ߴ����Ϊ20*20
		resize(dst_mat[i],dst_mat[i],Size(20,20));

	}
    printf("%s: %d ++\n", __FUNCTION__, __LINE__);

	for (int i = 1; i< char_mat.size();++i)
	{
		srcTri[0] = Point2f( 0,0 );  
		srcTri[1] = Point2f( char_mat[i].cols - 1, 0 );  
		srcTri[2] = Point2f( 0, char_mat[i].rows - 1 );
		length = char_mat[i].rows > char_mat[i].cols?char_mat[i].rows:char_mat[i].cols;
		dstTri[0] = Point2f( 0.0, 0.0 );  
		dstTri[1] = Point2f( length, 0.0 );  
		dstTri[2] = Point2f( 0.0, length ); 
		train_mat = getAffineTransform( srcTri, dstTri );
		dst_mat[i]=Mat::zeros(length,length,char_mat[i].type());		
		warpAffine(char_mat[i],dst_mat[i],train_mat,dst_mat[i].size(),INTER_LINEAR,BORDER_CONSTANT,Scalar(0));
		//resize(dst_mat[i],dst_mat[i],Size(20,20),0,0,CV_INTER_CUBIC);  //�ߴ����Ϊ20*20
		resize(dst_mat[i],dst_mat[i],Size(20,20));

	}
    printf("%s: %d ++\n", __FUNCTION__, __LINE__);

	/*for ( int i =0;i < char_mat.size();i++ )
	{
		int h=char_mat[i].rows;  
		int w=char_mat[i].cols;  
		int charSize=20;    //ͳһÿ���ַ��Ĵ�С  
		Mat transformMat=Mat::eye(2,3,CV_32F);  
		int m=max(w,h);  
		transformMat.at<float>(0,2)=m/2 - w/2;  
		transformMat.at<float>(1,2)=m/2 - h/2;  

		Mat warpImage(m,m, char_mat[i].type());  
		warpAffine(char_mat[i], warpImage, transformMat, warpImage.size(), INTER_LINEAR, BORDER_CONSTANT, Scalar(0) );  

		vector <Mat> dst_mat;  
		resize(warpImage, dst_mat[i], Size(charSize, charSize) );   
	}
  */
 	return 0;
 }

}
void ann_train(Ptr<ANN_MLP> &ann ,int numCharacters, int nlayers)
{
	Mat trainData ,classes;
	FileStorage fs;
    bool b_ret;

    b_ret = fs.open(FILE_ANN_XML, FileStorage::READ);
    if(b_ret != true)
    {
        printf("file %s open failed!\n", FILE_ANN_XML);
        return ;
    }

	fs["TrainingData"] >>trainData;
	fs["classes"] >>classes;

	//CvANN_MLP bp;   
	// Set up BPNetwork's parameters  
	//CvANN_MLP_TrainParams params;  
	//params.train_method=CvANN_MLP_TrainParams::BACKPROP;  
	//params.bp_dw_scale=0.1;  
	//params.bp_moment_scale=0.1; 

	Mat layerSizes(1,3,CV_32SC1);
	layerSizes.at<int>( 0 ) = trainData.cols;
	layerSizes.at<int>( 1 ) = nlayers; //������Ԫ��������Ϊ3
	layerSizes.at<int>( 2 ) = numCharacters; //��������Ϊ34
	//layerSizes.at<int>( 3 ) = numCharacters ;
	//ann->create(layerSizes , ANN_MLP::SIGMOID_SYM );  //��ʼ��ann
	ann->setLayerSizes(layerSizes);
	ann->setActivationFunction(ANN_MLP::SIGMOID_SYM);
	Mat trainClasses;
	trainClasses.create(trainData.rows , numCharacters ,CV_32FC1);
	for (int i =0;i< trainData.rows; i++)
	{
		for (int k=0 ; k< trainClasses.cols ; k++ )
		{
			if ( k == (int)classes.at<uchar> (i))
			{
				trainClasses.at<float>(i,k)  = 1 ;
			}
			else
				trainClasses.at<float>(i,k)  = 0;			
		}		
	}


	ann->train( trainData ,ROW_SAMPLE , trainClasses);

}

void ann_train1(Ptr<ANN_MLP> &ann ,int numCharacters, int nlayers)
{
	Mat trainData ,classes;
	FileStorage fs;
    bool b_ret;

    b_ret = fs.open(FILE_ANN_CHAR_XML , FileStorage::READ);
    if(b_ret == false)
    {
        printf("file %s open failed!\n", FILE_ANN_CHAR_XML);
        return ;
    }

	fs["TrainingData"] >>trainData;
	fs["classes"] >>classes;

	//CvANN_MLP bp;   
	// Set up BPNetwork's parameters  
	//CvANN_MLP_TrainParams params;  
	//params.train_method=CvANN_MLP_TrainParams::BACKPROP;  
	//params.bp_dw_scale=0.1;  
	//params.bp_moment_scale=0.1; 

	Mat layerSizes(1,3,CV_32SC1);
	layerSizes.at<int>( 0 ) = trainData.cols;
	layerSizes.at<int>( 1 ) = nlayers; //������Ԫ��������Ϊ3
	layerSizes.at<int>( 2 ) = numCharacters; //��������Ϊ34
	//layerSizes.at<int>( 3 ) = numCharacters ;
	ann->setLayerSizes(layerSizes);
	ann->setActivationFunction(ANN_MLP::SIGMOID_SYM);

	// Mat trainClasses;
	// trainClasses.create(trainData.rows , numCharacters ,CV_32FC1);
	// for (int i =0;i< trainData.rows; i++)
	// {
	// 	for (int k=0 ; k< trainClasses.cols ; k++ )
	// 	{
	// 		if ( k == (int)classes.at<uchar> (i))
	// 		{
	// 			trainClasses.at<float>(i,k)  = 1 ;
	// 		}
	// 		else
	// 			trainClasses.at<float>(i,k)  = 0;			
	// 	}		
	// }

	//Mat weights(1 , trainData.rows , CV_32FC1 ,Scalar::all(1) );
	ann->train( trainData ,ROW_SAMPLE , classes);
	//ann_train(trainData,trainData,params);
}


car_license_recogn::car_license_recogn(void)
{
	printf("%s: enter ++\n", __FUNCTION__);
}

int car_license_recogn::car_recogn_init(void)
{

    buf_size = FRAME_BUF_SIZE;
    image_buf = (unsigned char *)malloc(buf_size);
    if(image_buf == NULL)
    {
        buf_size = 0;
        printf("ERROR: malloc for video_buf failed!");
    }


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
	//图像数据列需要考虑通道数的影响
	int nCols = hsvImg.cols * channels;

	if (hsvImg.isContinuous())//连续存储的数据，按一行处理
	{
		nCols *= nRows;
		nRows = 1;
	}

	int i, j;
	uchar* p;

	const float  minref_sv = 64; //参考的S V的值
	const float max_sv = 255; // S V 的最大值

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
	Mat element = getStructuringElement(MORPH_RECT ,Size(17 ,3));  //闭形态学的结构元素
	morphologyEx(src_grey ,img_threshold,MORPH_CLOSE,element); 
	morphologyEx(img_threshold,img_threshold,MORPH_OPEN,element);//形态学处理

	vector< vector <Point> > contours;//寻找车牌区域的轮廓
	findContours(img_threshold ,contours,RETR_EXTERNAL, CHAIN_APPROX_NONE);//只检测外轮廓
	//对候选的轮廓进行进一步筛选
	vector< vector <Point> > ::iterator itc = contours.begin();
	while( itc != contours.end())
	{
		RotatedRect mr = minAreaRect(Mat( *itc )); //返回每个轮廓的最小有界矩形区域
		if(!verifySizes_closeImg(mr))  //判断矩形轮廓是否符合要求
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

	vector <Mat> output_area;
	normalPosArea(image ,rects, output_area);  //获得144*33的候选车牌区域output_area

	if(output_area.empty())
	{
		printf("%s: output_area is empty!\n", __FUNCTION__);
		return -1;
	}

	plate_mat = output_area[0];
	//imshow("plate mat", plate_mat);

	return 0;
}

int car_license_recogn::car_plate_recogn(void)
{
	vector<Mat> plates_svm;
	int ret;

	plates_svm.push_back(plate_mat);

    printf("%s: %d ++\n", __FUNCTION__, __LINE__);
    vector <Mat> char_seg;
	ret = char_segment(plates_svm[0],char_seg); 
	if(ret != 0)
	{
		return -1;
	}
    printf("%s: %d ++\n", __FUNCTION__, __LINE__);

	imwrite("char10.jpg",char_seg[0]);
	imwrite("char11.jpg",char_seg[1]);
	imwrite("char12.jpg",char_seg[2]);
	imwrite("char13.jpg",char_seg[3]);
	imwrite("char14.jpg",char_seg[4]);
	imwrite("char15.jpg",char_seg[5]);
	imwrite("char16.jpg",char_seg[6]);

	vector <Mat> char_feature;
	char_feature.resize(7);
	for (int i =0;i<char_seg.size() ;++ i)
		features(char_seg[i], char_feature[i],5);

	Ptr< ANN_MLP > ann_classify = ANN_MLP::create();
	ann_train(ann_classify,34,20);   //34Ϊ�������������48Ϊ���ز����Ԫ��

	Ptr< ANN_MLP > ann_classify1 = ANN_MLP::create();
	ann_train1(ann_classify1,31,30);

    //�ַ�Ԥ��
	vector<int>  char_result;
	//classify(ann_classify,char_feature,char_result);

	char_result.resize(char_feature.size());
	for (int i=0;i<char_feature.size(); ++i)
	{
		if (i==0)
	  {
			Mat output(1 ,31, CV_32FC1); //1*34����
			//ann.predict(char_feature[i] ,output);
			// Mat img = copyMakeBorder(char_feature[i], 1, 1, 5, 5, BORDER_CONSTANT , 0);
			Mat img;
			resize(char_feature[i], img, Size(8, 16), (0, 0), (0, 0), INTER_AREA);
			img = img.reshape(0,1);
			ann_classify1->predict(img,output);
			Point maxLoc;
			double maxVal;
			minMaxLoc(output , 0 ,&maxVal , 0 ,&maxLoc);
			char_result[i] =  maxLoc.x;
		   }
		else
		{
			Mat output(1 ,34, CV_32FC1); //1*34����
			//ann.predict(char_feature[i] ,output);
			ann_classify->predict(char_feature[i],output);
			Point maxLoc;
			double maxVal;
			minMaxLoc(output , 0 ,&maxVal , 0 ,&maxLoc);
			char_result[i] =  maxLoc.x;
		}
		
	}

   if(plates_svm.size() != 0)  
	{
        //imshow("Test", plates_svm[0]);     //��ȷԤ��Ļ�����ֻ��һ�����plates_svm[0]
	}
	else
	{
		std::cout<<"定位失败";
		return -1;
		
	}

    cout<<"该车牌后7位为：";
	char  s[] = {'0','1','2','3','4','5','6','7','8','9','A','B',
		'C','D','E','F','G','H','J','K','L','M','N','P','Q',
		'R','S','T','U','V','W','X','Y','Z'};//���������˾�
	cout<<'\n';	

	string chinese[]= {"川","鄂","赣","甘","贵","桂","黑","沪","冀","津","京","吉","辽",
	"鲁","蒙","闽","宁","青","琼","陕","苏","晋","皖","湘","新","豫","渝","粤","云","藏","浙"};

	char car_num[16] = {0};
	for (int w=0;w<char_result.size(); w++)   //��һλ�Ǻ��֣�����ûʵ�ֶԺ��ֵ�Ԥ��
	{     
		if (w==0)
		{
			cout<<chinese[char_result[w]];//���ԶԺ��־���ʶ��
			cout<<'\t';
			memcpy(car_num, chinese[char_result[w]].c_str(), 3);
		    }
		else
		{
			cout<< s[char_result[w]];
			car_num[w+2] = s[char_result[w]];
			cout<<'\t';
		    }
	}
    printf("%s: %d ++\n", __FUNCTION__, __LINE__);

    history_info_t hist_info = {0};
    QTime time = QTime::currentTime();
    hist_info.time = time.hour()*10000 + time.minute()*100 + time.second();
    strncpy(hist_info.license, car_num, sizeof(hist_info.license));
    sql_history_add(mainwindow->sql_db, &hist_info);

    //mainwindow->mainwin_set_cartext(car_num);

	cout<<endl;
 	//�˺����ȴ�������������������ͷ���
	//svmClassifier.clear();
	//imshow("plate",plates_svm[0]);
	//waitKey(0);
	//svmClassifier->clear();

	return 0;
}

void *opencv_car_recogn_thread(void *arg)
{
	class car_license_recogn *car_recogn = &car_recogn_unit;
	QImage tmpQImage;
	Mat captureMat;
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

        ret = videoframe_get_one(car_recogn->image_buf, FRAME_BUF_SIZE, &frame_len);
		if(ret != 0)
		{
			usleep(100 *1000);
			continue;
		}
		//printf("frame_buf: %p, frame_len: %d\n", frame_buf, frame_len);

		/* 将v4l2获取到的MJPEG图像转换为QImage格式 */
        tmpQImage = jpeg_to_QImage(car_recogn->image_buf, frame_len);
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

        //imwrite("error.jpg", captureMat);
        //captureMat = imread("error.jpg");
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
        printf("detect car plate\n");

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


