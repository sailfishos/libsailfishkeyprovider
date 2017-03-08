TEMPLATE = lib
TARGET = sailfishkeyprovider
TARGET = $$qtLibraryTarget($$TARGET)
TARGETPATH = /usr/lib
target.path = $$TARGETPATH

CONFIG -= qt
MOC_DIR = $$PWD/../.moc
OBJECTS_DIR = $$PWD/../.obj

include($$PWD/lib.pri)

includes.path = /usr/include/libsailfishkeyprovider
includes.files = \
    $$PWD/include/sailfishkeyprovider.h \
    $$PWD/include/sailfishkeyprovider_iniparser.h \
    $$PWD/include/sailfishkeyprovider_processmutex.h

packageconfig.path = /usr/lib/pkgconfig
packageconfig.files = $$PWD/pkgconfig/libsailfishkeyprovider.pc

INSTALLS += target includes packageconfig
