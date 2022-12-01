#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <iostream>
#include "mainwindow.h"
#include "image_convert.h"
#include "config.h"


/* C++ include C */
#ifdef __cplusplus
extern "C" {
#endif
#include "capture.h"
#ifdef __cplusplus
}
#endif


static MainWindow *mainwindow;


MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{

    // 设置窗口标题
	setWindowTitle(DEFAULT_WINDOW_TITLE);

	resize(DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT);

	mainWindow = new QWidget;
	setCentralWidget(mainWindow);

   	backgroundImg.load(WIN_BACKGROUND_IMG);

    // 背景区域
	videoArea = new QLabel(mainWindow);
	videoArea->setPixmap(QPixmap::fromImage(backgroundImg));
	videoArea->setGeometry(0, 0, 640, 480);
	videoArea->show();

	carPlateArea = new QLabel(mainWindow);
	carPlateArea->setGeometry(640, 0, 160, 80);
	carPlateArea->show();

	/* set timer to show image */
	timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), this, SLOT(window_display()));
	timer->start(TIMER_INTERV_MS);

}

MainWindow::~MainWindow(void)
{
	
}

void MainWindow::window_display(void)
{
	unsigned char *frame_buf = NULL;
	int frame_len = 0;
	int len = 0;
	int ret;

	timer->stop();

	/* show capture image */
	ret = capture_get_framebuf(&frame_buf, &frame_len);
	if(ret == 0)
	{
		QImage videoQImage;

		videoQImage = jpeg_to_QImage(frame_buf, frame_len);
		capture_put_framebuf();

		videoArea->setPixmap(QPixmap::fromImage(videoQImage));
		videoArea->show();

		if(!plateImg.isNull())
		{
			carPlateArea->setPixmap(QPixmap::fromImage(plateImg));
			carPlateArea->show();
		}
	}

	timer->start(TIMER_INTERV_MS);

}


int mainwindow_init(void)
{
	mainwindow = new MainWindow;

	mainwindow->show();
	
	return 0;
}

int mainwin_set_plateImg(QImage &plateImg)
{

	mainwindow->plateImg = plateImg;

	return 0;
}

/* notice:
 * use timer to display,
 * if use thread, it will occur some error.
 */
int start_mainwindow_task(void)
{
	mainwindow_init();

	return 0;
}


