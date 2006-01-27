TEMPLATE = app
LANGUAGE = C++
TARGET   = NifSkope

CONFIG += qt release warn_on
QT += xml opengl
#LIBS += -lmingw32 -lqtmain

DESTDIR = ./

HEADERS += \
glcontroller.h \
glscene.h \
glview.h \
nifdelegate.h \
nifmodel.h \
nifproxy.h \
nifskope.h \
niftypes.h \
nifview.h \
spellbook.h \
spells/*.h

SOURCES += \
glcontroller.cpp \
glscene.cpp \
gltex.cpp \
glview.cpp \
nifdelegate.cpp \
nifmodel.cpp \
nifproxy.cpp \
nifskope.cpp \
niftypes.cpp \
nifview.cpp \
nifxml.cpp \
spellbook.cpp \
spells/*.cpp

RESOURCES += nifskope.qrc

win32 {
    RC_FILE = icon.rc
}
