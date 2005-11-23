TARGET = NifSkope

TEMPLATE = app

SOURCES += *.cpp
HEADERS += *.h

CONFIG += qt release warn_on
QT += XML OpenGL

DESTDIR = $$PWD

win32:debug {
CONFIG += console
}
