QT       += core gui

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
    CCustomTitle.cpp \
    CVideoPlayer.cpp \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    CCustomTitle.h \
    CVideoPlayer.h \
    mainwindow.h

FORMS += \
    CCustomTitle.ui \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

#将输出文件直接放到源码目录上一级的win的bin目录下，将dll都放在了次目录中，用以解决运行后找不到dll的问#DESTDIR=$$PWD/bin/
contains(QT_ARCH, i386) {
    message("32-bit")
    DESTDIR = $${PWD}/../win/bin32

    message($$DESTDIR)
} else {
    message("64-bit")
    DESTDIR = $${PWD}/../win/bin64
}

win32{

    contains(QT_ARCH, i386) {
    message("32-bit")
    QMAKE_LFLAGS += -shared
    INCLUDEPATH += $$PWD/../win/bin32/include
    INCLUDEPATH += $$PWD/../win/bin32/include/SDL2

    LIBS += -L$$PWD/../win/bin32/lib -lavcodec -lavdevice -lavfilter -lavformat -lavutil -lpostproc -lswresample -lswscale
    LIBS += -L$$PWD/../win/bin32/lib -lSDL2
} else {
    message("64-bit")
    INCLUDEPATH += $$PWD/../win/bin64/include
    $$PWD/src

    LIBS += -L$$PWD/../win/bin64/lib -lavcodec -lavdevice -lavfilter -lavformat -lavutil -lpostproc -lswresample -lswscale
}

}

