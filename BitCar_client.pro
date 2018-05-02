#------------------------------
#
# Project created by QtCreator
#
#------------------------------

QT    += core gui declarative
QT    += widgets
TARGET = CameraCalibration
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    cameracalibrator.cpp

HEADERS  += mainwindow.h \
    cameracalibrator.h

FORMS    += mainwindow.ui

## SET THE CORRECT PATH HEREAFTER ##

INCLUDEPATH += .
INCLUDEPATH += /usr/include
INCLUDEPATH += /usr/include/opencv2
#INCLUDEPATH += /opt/ros/kinetic/include/opencv-3.3.1-dev/opencv2
#INCLUDEPATH += /opt/ros/kinetic/include/opencv-3.3.1-dev

LIBS += `pkg-config opencv --cflags --libs`
#LIBS += `pkg-config opencv-3.3.1-dev --cflags --libs`
