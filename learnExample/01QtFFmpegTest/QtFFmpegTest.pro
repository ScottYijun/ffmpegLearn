TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

#将输出文件直接放到源码目录下的bin目录下，将dll都放在了次目录中，用以解决运行后找不到dll的问#DESTDIR=$$PWD/bin/
contains(QT_ARCH, i386) {
message("32-bit")
DESTDIR = $${PWD}/../win/bin32
} else {
message("64-bit")
DESTDIR = $${PWD}/../win/bin64
}

win32{

contains(QT_ARCH, i386) {
message("32-bit")
INCLUDEPATH += $$PWD/../win/bin32/include
$$PWD/src

LIBS += -L$$PWD/../win/bin32/lib -lavcodec -lavdevice -lavfilter -lavformat -lavutil -lpostproc -lswresample -lswscale

} else {
message("64-bit")
INCLUDEPATH += $$PWD/../win/bin64/include
$$PWD/src

LIBS += -L$$PWD/../win/bin64/lib -lavcodec -lavdevice -lavfilter -lavformat -lavutil -lpostproc -lswresample -lswscale
}

}

SOURCES += \
        main.cpp
