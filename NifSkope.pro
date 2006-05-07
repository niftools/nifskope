TEMPLATE = app
LANGUAGE = C++
TARGET   = NifSkope

CONFIG += qt release thread warn_on
QT += xml opengl
#LIBS += -lmingw32 -lqtmain

DESTDIR = ./

HEADERS += \
basemodel.h \
kfmmodel.h \
nifmodel.h \
glview.h \
message.h \
nifproxy.h \
nifskope.h \
niftypes.h \
nifview.h \
spellbook.h \
gl/*.h \
widgets/*.h \
spells/*.h

SOURCES += \
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
niftypes.cpp \
nifview.cpp \
spellbook.cpp \
gl/*.cpp \
widgets/*.cpp \
spells/*.cpp \
NvTriStrip/*.cpp

RESOURCES += nifskope.qrc

win32 {
    RC_FILE = icon.rc
}
