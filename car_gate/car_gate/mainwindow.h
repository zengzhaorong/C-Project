#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE


#define TIMER_INTERV_MS			1
#define BACKGROUND_IMAGE    "../car_gate/background.png"


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();


private slots:
    void window_display(void);

private:
    Ui::MainWindow *ui;

    QTimer 			*timer;				// display timer

};

int mainwindow_init(void);

#endif // MAINWINDOW_H
