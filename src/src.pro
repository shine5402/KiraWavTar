QT       += core gui concurrent

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17
VERSION = 1.0.3

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    extracttargetselectmodel.cpp \
    main.cpp \
    mainwindow.cpp \
    wavcombine.cpp \
    wavcombinedialog.cpp \
    wavextract.cpp \
    wavextractdialog.cpp \
    wavformatchooserwidget.cpp \
    wavtar_utils.cpp

HEADERS += \
    extracttargetselectmodel.h \
    mainwindow.h \
    wavcombine.h \
    wavcombinedialog.h \
    wavextract.h \
    wavextractdialog.h \
    wavformatchooserwidget.h \
    wavtar_utils.h

FORMS += \
    mainwindow.ui \
    wavformatchooserwidget.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

include(external-libs.pri)

# KiraCommonUtils

win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../lib/KiraCommonUtils/src/release/ -lKiraCommonUtils
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../lib/KiraCommonUtils/src/debug/ -lKiraCommonUtils
else:unix: LIBS += -L$$OUT_PWD/../lib/KiraCommonUtils/src/ -lKiraCommonUtils

INCLUDEPATH += $$PWD/../lib/KiraCommonUtils/src/include
DEPENDPATH += $$PWD/../lib/KiraCommonUtils/src/include

win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../lib/KiraCommonUtils/src/release/libKiraCommonUtils.a
else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../lib/KiraCommonUtils/src/debug/libKiraCommonUtils.a
else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../lib/KiraCommonUtils/src/release/KiraCommonUtils.lib
else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../lib/KiraCommonUtils/src/debug/KiraCommonUtils.lib
else:unix: PRE_TARGETDEPS += $$OUT_PWD/../lib/KiraCommonUtils/src/libKiraCommonUtils.a
