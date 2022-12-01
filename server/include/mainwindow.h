#ifndef _MAINWINDOW_H_
#define _MAINWINDOW_H_

#include <QMainWindow>
#include <QWidget>
#include <QLabel>
#include <QTimer>
#include <QPushButton>
#include <QLineEdit>
#include <QTextCodec>
#include <QComboBox>
#include <QMessageBox>
#include <QTableView>
#include <QHeaderView>
#include <QStandardItemModel>
#include <QDateTime>
#include <QDateTimeEdit>
#include <QPainter>
#include <QRect>

/* C++ include C */
#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

#define TIMER_INTERV_MS			1

#define WIN_BACKGROUND_IMG  	"resource/background"		// 界面背景图


class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	MainWindow(QWidget *parent = 0);
	~MainWindow();

private slots:
	void window_display(void);
	
public:
	QImage			plateImg;			// car plate image

private:
	QWidget			*mainWindow;		// main window
	QLabel 			*videoArea;			// video area
	QLabel 			*carPlateArea;		// car plate
	QImage			backgroundImg;		// background image
	QTimer 			*timer;				// display timer
	
};


int mainwin_set_plateImg(QImage &plateImg);
int start_mainwindow_task(void);


#endif	// _MAINWINDOW_H_