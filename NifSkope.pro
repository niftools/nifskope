TEMPLATE = app
LANGUAGE = C++
TARGET   = NifSkope

CONFIG += qt release warn_on
QT += xml opengl
#LIBS += -lmingw32 -lqtmain

DESTDIR = ./

HEADERS += \
widgets\*.h \
glcontrolable.h \
glcontroller.h \
gllight.h \
glmesh.h \
glnode.h \
glnodeswitch.h \
glparticles.h \
glproperty.h \
glscene.h \
gltex.h \
gltransform.h \
glview.h \
message.h \
nifmodel.h \
nifproxy.h \
nifskope.h \
niftypes.h \
nifview.h \
spellbook.h \
spells/*.h

SOURCES += \
widgets\*.cpp \
glcontroller.cpp \
gllight.cpp \
glmesh.cpp \
glnode.cpp \
glnodeswitch.cpp \
glparticles.cpp \
glproperty.cpp \
glscene.cpp \
gltex.cpp \
gltransform.cpp \
glview.cpp \
message.cpp \
nifdelegate.cpp \
nifmodel.cpp \
nifproxy.cpp \
nifskope.cpp \
niftypes.cpp \
nifview.cpp \
nifxml.cpp \
spellbook.cpp \
spells/*.cpp \
NvTriStrip/*.cpp

RESOURCES += nifskope.qrc

win32 {
    RC_FILE = icon.rc
}
