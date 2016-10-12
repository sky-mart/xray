#-------------------------------------------------
#
# Project created by QtCreator 2016-10-11T14:25:07
#
#-------------------------------------------------

QT       += core gui serialport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Dashboard
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
        wiener.cpp

HEADERS  += mainwindow.h \
        wiener.h

FORMS    += mainwindow.ui

win32 {
message("Using win32 configuration")

OPENCV_PATH = D:/OpenCV/opencv # Note: update with the correct OpenCV version


#LIBS_PATH = "$$OPENCV_PATH/build/x86/mingw/lib" #project compiled using MINGW
LIBS_PATH = "$$OPENCV_PATH/build/x86/vc12/lib" #project compiled using Visual C++ 2010 32bit compiler

    CONFIG(debug, debug|release) {
    LIBS     += -L$$LIBS_PATH \
                -lopencv_core2413d \
                -lopencv_highgui2413d \
                -lopencv_imgproc2413d
    }

    CONFIG(release, debug|release) {
    LIBS     += -L$$LIBS_PATH \
                -lopencv_core2413 \
                -lopencv_highgui2413  \
                -lopencv_imgproc2413
    }
}

unix {
message("Using unix configuration")

OPENCV_PATH = /usr/opencv2

LIBS_PATH = /usr/lib

LIBS     += \
    -L$$LIBS_PATH \
    -lopencv_core \
    -lopencv_highgui
}

INCLUDEPATH += \
    $$OPENCV_PATH/build/include/

message("OpenCV path: $$OPENCV_PATH")
message("Includes path: $$INCLUDEPATH")
message("Libraries: $$LIBS")
