#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <opencv2/opencv.hpp>
#include "opencv_car_recogn.h"


/* C++ include C */
#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

using namespace std;
using namespace cv;


class car_license_recogn    car_recogn_unit;


car_license_recogn::car_license_recogn(void)
{
	printf("%s: enter ++\n", __FUNCTION__);
}


void *opencv_car_recogn_thread(void *arg)
{
    printf("### %s enter ++\n", __FUNCTION__);

    while(1)
    {
        usleep(100);
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


