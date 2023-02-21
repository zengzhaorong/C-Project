QT       += core gui sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    image_convert.cpp \
    main.cpp \
    mainwindow.cpp \
    opencv_car_recogn.cpp \
    protocol.cpp \
    ringbuffer.cpp \
    socket_server.cpp \
    sql_db.cpp

HEADERS += \
    config.h \
    image_convert.h \
    mainwindow.h \
    opencv_car_recogn.h \
    protocol.h \
    ringbuffer.h \
    socket_server.h \
    sql_db.h

FORMS += \
    mainwindow.ui

LIB_OPENCV_PATH = /usr/local/opencv4

INCLUDEPATH += $${LIB_OPENCV_PATH}/include  $${LIB_OPENCV_PATH}/include/opencv4

LIBS += -L$${LIB_OPENCV_PATH}/lib/ -lopencv_core -lopencv_objdetect \
        -lopencv_highgui -lopencv_imgproc \
        -lopencv_imgcodecs -lopencv_ml

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
