QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    dirnameeditwithbrowse.cpp \
    filenameeditwithbrowse.cpp \
    main.cpp \
    mainwindow.cpp \
    wavcombine.cpp \
    wavextract.cpp \
    wavtar_utils.cpp

HEADERS += \
    dirnameeditwithbrowse.h \
    filenameeditwithbrowse.h \
    kfr_adapt.h \
    mainwindow.h \
    wavcombine.h \
    wavextract.h \
    wavtar_utils.h

FORMS += \
    filenameeditwithbrowse.ui \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

include(external-libs.pri)
