#include "mainwindow.h"
#include "socket_server.h"
#include "opencv_car_recogn.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    mainwindow_init();

    // 设置stdout缓冲大小为0，使得printf马上输出
    setbuf(stdout, NULL);

    start_socket_server_task();

    start_car_recogn_task();

    return a.exec();
}
