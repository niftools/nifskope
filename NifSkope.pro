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
nifproxy.h \
nifskope.h \
niftypes.h \
nifview.h \
spellbook.h \
widgets/*.h \
spells/*.h

SOURCES += \
basemodel.cpp \
kfmmodel.cpp \
kfmxml.cpp \
nifmodel.cpp \
nifxml.cpp \
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
nifproxy.cpp \
nifskope.cpp \
niftypes.cpp \
nifview.cpp \
spellbook.cpp \
widgets/*.cpp \
spells/*.cpp \
NvTriStrip/*.cpp

RESOURCES += nifskope.qrc

win32 {
    RC_FILE = icon.rc
}
