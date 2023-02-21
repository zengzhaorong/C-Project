#include "mainwindow.h"
#include "capture.h"
#include <unistd.h>
#include "socket_client.h"
#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    mainwindow_init();

    // 设置stdout缓冲大小为0，使得printf马上输出
    setbuf(stdout, NULL);

    newframe_mem_init();

    sleep(1);	// only to show background image
    start_capture_task();

    start_socket_client_task();

    return a.exec();
}
