TEMPLATE = app
TARGET = sailfish-keyprovider-keygen

target.path = /usr/bin
INSTALLS += target

CONFIG -= qt

include($$PWD/../lib/lib.pri)
SOURCES += keygen.c
