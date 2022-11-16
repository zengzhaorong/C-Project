#include <iostream>
#include <unistd.h>
#include <QApplication>
#include "mainwindow.h"
#include "opencv_car_recogn.h"


/* C++ include C */
#ifdef __cplusplus
extern "C" {
#endif
#include "capture.h"
#ifdef __cplusplus
}
#endif

using namespace std;

int main(int argc, char* argv[])
{
    QApplication qtApp(argc, argv);
	(void)argc;
	(void)argv;

	cout << "hello server main" << endl;

	start_mainwindow_task();
	
	newframe_mem_init();

	sleep(1);	// only to show background image
	start_capture_task();

	// 启动车牌识别任务
	start_car_recogn_task();

	return qtApp.exec();		// start qt application, message loop ...
}
