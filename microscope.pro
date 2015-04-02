#-------------------------------------------------
#
# Project created by QtCreator 2013-09-18T17:33:49
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = microscope
TEMPLATE = app

QMAKE_CXXFLAGS += --std=gnu++0x

SOURCES += main.cpp\
        mainwindow.cpp \
    microscope.cpp \
    cv_center_of_mass.cpp \
    mikroscope_ann_cal_builder.cpp \
    ann.cpp

HEADERS  += mainwindow.h \
    microscope.h \
    cv_center_of_mass.h \
    mikroscope_ann_cal_builder.h \
    ann.h

FORMS    += mainwindow.ui

LIBS += \
    -lopencv_core \
    -lopencv_videostab \
    -lopencv_objdetect \
    -lopencv_contrib \
    -lopencv_highgui \
    -lopencv_legacy \
    -lopencv_video \
    -lopencv_videostab \
    -lopencv_legacy \
    -lopencv_calib3d \
    -lopencv_features2d \
    -lopencv_flann \
    -lopencv_imgproc \
    -lopencv_stitching \
    -lopencv_flann \
    -lopencv_ml \
    -lopencv_photo \
    -lopencv_objdetect \
    -lopencv_features2d \
    -lopencv_ts \
    -lopencv_contrib \
    -lopencv_highgui \
    -lopencv_ml \
    -lopencv_stitching \
    -lopencv_photo \
    -lopencv_imgproc \
    -lopencv_calib3d \
    -lopencv_core \
    -lopencv_ts \
    -lopencv_video
