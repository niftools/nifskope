TEMPLATE = app
LANGUAGE = C++
TARGET   = bsatest

DEFINES += BSA_TEST

CONFIG += qt release thread warn_on console
LIBS += -lmingw32 -lqtmain

DESTDIR = ./

HEADERS += *.h
SOURCES += *.cpp

# vim: set filetype=config : 
