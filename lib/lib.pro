TEMPLATE = lib
TARGET = sailfishkeyprovider
TARGET = $$qtLibraryTarget($$TARGET)
TARGETPATH = /usr/lib
target.path = $$TARGETPATH

CONFIG -= qt
CONFIG += create_pc create_prl no_install_prl
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

QMAKE_PKGCONFIG_NAME = lib$$TARGET
QMAKE_PKGCONFIG_DESCRIPTION = Provides decoded service keys
QMAKE_PKGCONFIG_LIBDIR = $$target.path
QMAKE_PKGCONFIG_INCDIR = $$includes.path
QMAKE_PKGCONFIG_DESTDIR = pkgconfig
QMAKE_PKGCONFIG_VERSION = $$VERSION
QMAKE_PKGCONFIG_FILE = lib$$TARGET

INSTALLS += target includes packageconfig
