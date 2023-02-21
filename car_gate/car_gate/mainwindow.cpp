#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "capture.h"


static MainWindow *mainwindow;

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


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    /* set window title - 设置窗口标题 */
    setWindowTitle(DEFAULT_WINDOW_TITLE);

    // background image
    QImage bg_image;
    bg_image.load(BACKGROUND_IMAGE);
    ui->videoArea->setPixmap(QPixmap::fromImage(bg_image));



    /* set timer to show image */
    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(window_display()));
    timer->start(TIMER_INTERV_MS);

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::window_display(void)
{
    unsigned char *frame_buf = NULL;
    int frame_len = 0;
    int ret;

    timer->stop();

    /* show capture image */
    ret = capture_get_framebuf(&frame_buf, &frame_len);
    if(ret == 0)
    {
        QImage videoQImage;

        videoQImage = jpeg_to_QImage(frame_buf, frame_len);
        capture_put_framebuf();

        ui->videoArea->setPixmap(QPixmap::fromImage(videoQImage));
        ui->videoArea->show();

    }

    timer->start(TIMER_INTERV_MS);

}

/* main window initial - 主界面初始化 */
int mainwindow_init(void)
{
    mainwindow = new MainWindow;

    mainwindow->show();

    return 0;
}


