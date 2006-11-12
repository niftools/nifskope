TEMPLATE = app
LANGUAGE = C++
TARGET   = NifSkope

CONFIG += qt release thread warn_on
QT += xml opengl network
#LIBS += -lmingw32 -lqtmain

#DEFINES += USE_GL_QPAINTER

DESTDIR = ./

HEADERS += \
niftypes.h \
nifvalue.h \
basemodel.h \
kfmmodel.h \
nifmodel.h \
glview.h \
message.h \
nifproxy.h \
nifskope.h \
spellbook.h \
gl/*.h \
widgets/*.h \
spells/*.h

SOURCES += \
niftypes.cpp \
nifvalue.cpp \
basemodel.cpp \
kfmmodel.cpp \
kfmxml.cpp \
nifmodel.cpp \
nifxml.cpp \
glview.cpp \
message.cpp \
nifdelegate.cpp \
nifproxy.cpp \
nifskope.cpp \
spellbook.cpp \
gl/*.cpp \
widgets/*.cpp \
spells/*.cpp \
NvTriStrip/*.cpp

RESOURCES += nifskope.qrc

win32 {
    RC_FILE = icon.rc
}
