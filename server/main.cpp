#include <iostream>
#include <unistd.h>
#include <QApplication>
#include "mainwindow.h"


/* C++ include C */
#ifdef __cplusplus
extern "C" {
#endif
/* C head file */
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

	return qtApp.exec();		// start qt application, message loop ...
}
