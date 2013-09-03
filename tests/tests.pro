TEMPLATE=app
TARGET=tst_keyprovider
TARGETPATH = /opt/tests/libsailfishkeyprovider
target.path = $$TARGETPATH

CONFIG -= qt
MOC_DIR = $$PWD/../.moc
OBJECTS_DIR = $$PWD/../.obj

include($$PWD/../lib/lib.pri)
SOURCES += tst_keyprovider.c
INSTALLS += target
