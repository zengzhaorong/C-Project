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

	buf_size = FRAME_BUF_SIZE;
	video_buf = (unsigned char *)malloc(buf_size);
	if(video_buf == NULL)
	{
		buf_size = 0;
		printf("ERROR: malloc for video_buf failed!");
	}

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
	int len = 0;
	int ret;

	/* show capture image */
	ret = capture_get_newframe(video_buf, buf_size, &len);
	if(ret == 0)
	{
		QImage videoQImage;

		videoQImage = jpeg_to_QImage(video_buf, len);

		videoArea->setPixmap(QPixmap::fromImage(videoQImage));
		videoArea->show();
	}


}


int mainwindow_init(void)
{
	mainwindow = new MainWindow;

	mainwindow->show();
	
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


