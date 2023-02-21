#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSqlDatabase>
#include <QSqlTableModel>
#include <QTimer>
#include "config.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE


#define NAME_ID         "ID"
#define NAME_LICENSE    "车牌"

#define TIMER_INTERV_MS			1
#define BACKGROUND_IMAGE    "../car_platform/background.png"


struct videoframe_info
{
    uint8_t *frame_buf;
    uint32_t frame_len;
    pthread_mutex_t	mutex;
};


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    QImage plateImg;
    QSqlDatabase sql_db;
    QSqlTableModel *sqlmodel;

private slots:
    void window_display(void);

    void on_addCarBtn_clicked();

    void on_carDataBtn_clicked();

    void on_delCarBtn_clicked();

private:
    Ui::MainWindow *ui;

    int initTableView(void);
    QTimer 			*timer;				// display timer
    unsigned char 	*video_buf;
    unsigned int 	buf_size;

};


int videoframe_update(uint8_t *data, int len);
int videoframe_get_one(uint8_t *data, uint32_t size, int *len);
int videoframe_clear(void);
int videoframe_mem_init(void);

int mainwin_set_plateImg(QImage &plateImg);
int mainwindow_init(void);


#endif // MAINWINDOW_H
