#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "image_convert.h"
#include <QTime>
#include "sql_db.h"
#include "config.h"
#include <QDebug>

MainWindow *mainwindow;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    int ret;

    ui->setupUi(this);

    /* set window title - 设置窗口标题 */
    setWindowTitle(DEFAULT_WINDOW_TITLE);

    ui->tableView->setHidden(true);

    // background image
    QImage bg_image;
    bg_image.load(BACKGROUND_IMAGE);
    ui->videoArea->setPixmap(QPixmap::fromImage(bg_image));

    buf_size = FRAME_BUF_SIZE;
    video_buf = (unsigned char *)malloc(FRAME_BUF_SIZE);
    if(video_buf == NULL)
    {
        buf_size = 0;
        qDebug() << ("ERROR: malloc for video_buf failed!");
    }

    sql_db_init(sql_db);
    initTableView();
    ui->tableView->setModel(sqlmodel);

    car_info_t car_info;
    int total, cursor = 0;
    total = sql_car_get_total(sql_db);
    for(int i=0; i<total; i++)
    {
        ret = sql_car_traverse(sql_db, &cursor, &car_info);
        if(ret != 0)
            break;

        ui->carListBox->addItem(car_info.license);
        qDebug() << i << ": "<< car_info.id << "-" << car_info.license;
    }



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
    int len;
    int ret;

    timer->stop();

    /* get video frame data - 获取视频区域的图像 */
    ret = videoframe_get_one(video_buf, buf_size, &len);
    if(ret == 0)
    {
        QImage videoQImage;
        QPixmap pixmap;

        /* transform jpeg to QImage - 将jpeg格式的图像转换为QT格式的，为了用QT显示 */
        videoQImage = jpeg_to_QImage(video_buf, len);
        pixmap = QPixmap::fromImage(videoQImage);
        ui->videoArea->setPixmap(pixmap);	// set image data - 设置图像数据到控件
        //ui->videoArea->show();	// display - 显示控件

        if(!plateImg.isNull())
        {
            ui->plateArea->setPixmap(QPixmap::fromImage(plateImg));
        }
    }

    timer->start(TIMER_INTERV_MS);
}

int MainWindow::initTableView(void)
{
    bool bret;

    sqlmodel = new QSqlTableModel(this, sql_db);
    sqlmodel->setTable(TABLE_HISTORY);

    bret = sqlmodel->select();
    if(bret == false)
    {
        qDebug() << "sqlmodel select failed!";
        return -1;
    }


    sqlmodel->setHeaderData(sqlmodel->fieldIndex(DB_COL_ID), Qt::Horizontal, NAME_ID);
    sqlmodel->setHeaderData(sqlmodel->fieldIndex(DB_COL_LICENSE), Qt::Horizontal, NAME_LICENSE);

    return 0;
}

int mainwin_set_plateImg(QImage &plateImg)
{

    mainwindow->plateImg = plateImg;

    return 0;
}

void MainWindow::on_addCarBtn_clicked()
{
    QString licenStr;
    car_info_t car_info = {0};

    licenStr = ui->carLicenEdit->text();
    if(licenStr.length() <= 0)
    {
        qDebug() << "car license is null!";
        return ;
    }
    qDebug() << "add car license: " << licenStr;

    // toLocal8Bit(): Unicode编码
    strcpy(car_info.license, ui->carLicenEdit->text().toLocal8Bit().data());

    QTime time = QTime::currentTime();
    car_info.id = time.hour()*10000 + time.minute()*100 + time.second();

    sql_car_add(sql_db, &car_info);

    ui->carListBox->addItem(car_info.license);
}

void MainWindow::on_delCarBtn_clicked()
{
    QString licenStr;
    char license[32] = {0};
    int index;

    licenStr = ui->carListBox->currentText();
    if(licenStr.length() <= 0)
    {
        qDebug() << "car license is null!";
        return ;
    }
    qDebug() << "del car license: " << licenStr;

    strcpy(license, licenStr.toLocal8Bit().data());

    sql_car_del(sql_db, license);

    index = ui->carListBox->currentIndex();
    ui->carListBox->removeItem(index);
}

void MainWindow::on_carDataBtn_clicked()
{
    if(ui->carDataBtn->isChecked())
    {
        ui->tableView->setHidden(false);
    }
    else
    {
        ui->tableView->setHidden(true);
    }
}



/* main window initial - 主界面初始化 */
int mainwindow_init(void)
{
    mainwindow = new MainWindow;

    mainwindow->show();

    return 0;
}

