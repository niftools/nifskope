TEMPLATE = app
LANGUAGE = C++
TARGET   = NifSkope

CONFIG += qt release warn_on
QT += xml opengl

DESTDIR = ./

HEADERS += \
glmath.h \
glscene.h \
glview.h \
nifmodel.h \
nifproxy.h \
nifskope.h \
nifview.h \
popup.h

SOURCES += \
glscene.cpp \
gltex.cpp \
glview.cpp \
nifdelegate.cpp \
nifmodel.cpp \
nifproxy.cpp \
nifskope.cpp \
nifview.cpp \
nifxml.cpp \
popup.cpp

RESOURCES += nifskope.qrc

win32 {
    RC_FILE = icon.rc
}
