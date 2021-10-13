QT       += core gui concurrent

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17
VERSION = 0.1.3

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    dirnameeditwithbrowse.cpp \
    extracttargetselectmodel.cpp \
    filenameeditwithbrowse.cpp \
    main.cpp \
    mainwindow.cpp \
    wavcombine.cpp \
    wavcombinedialog.cpp \
    wavextract.cpp \
    wavextractdialog.cpp \
    wavformatchooserwidget.cpp \
    wavtar_utils.cpp

HEADERS += \
    dirnameeditwithbrowse.h \
    extracttargetselectmodel.h \
    filenameeditwithbrowse.h \
    kfr_adapt.h \
    mainwindow.h \
    wavcombine.h \
    wavcombinedialog.h \
    wavextract.h \
    wavextractdialog.h \
    wavformatchooserwidget.h \
    wavtar_utils.h

FORMS += \
    filenameeditwithbrowse.ui \
    mainwindow.ui \
    wavformatchooserwidget.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

INCLUDEPATH += libs/eternal/include/

include(external-libs.pri)
