INCLUDEPATH += \
    $$PWD/include \
    $$PWD/src

HEADERS += \
    $$PWD/include/sailfishkeyprovider.h \
    $$PWD/include/sailfishkeyprovider_iniparser.h \
    $$PWD/src/base64ed.h \
    $$PWD/src/xored.h

SOURCES += \
    $$PWD/src/sailfishkeyprovider.c \
    $$PWD/src/base64ed.c \
    $$PWD/src/xored.c \
    $$PWD/src/iniparser.c

OTHER_FILES += \
    $$PWD/pkgconfig/libsailfishkeyprovider.pc

