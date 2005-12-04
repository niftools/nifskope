TEMPLATE = app
LANGUAGE = C++
TARGET   = NifSkope

CONFIG += qt release warn_on
QT += xml opengl

DESTDIR = $$PWD

HEADERS   += *.h
SOURCES   += *.cpp
#RESOURCES += *.qrc

win32 {
    RC_FILE = icon.rc
}
