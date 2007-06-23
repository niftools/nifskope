TEMPLATE = app
LANGUAGE = C++
TARGET   = NifSkope

QT += xml opengl network

CONFIG += qt release thread warn_on

CONFIG += fsengine

# uncomment this if you want all the messages to be logged to stdout
#CONFIG += console

# uncomment this if you want the text stats gl option
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
    options.h \
    gl/*.h \
    gl/marker/*.h \
    widgets/*.h \
    spells/*.h \
    importex/*.h

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
    options.cpp \
    gl/*.cpp \
    widgets/*.cpp \
    spells/*.cpp \
    importex/*.cpp \
    NvTriStrip/*.cpp

RESOURCES += \
    nifskope.qrc

fsengine {
    DEFINES += FSENGINE
    HEADERS += fsengine/*.h
    SOURCES += fsengine/*.cpp
}

win32 {
    # useful for MSVC2005
    CONFIG += embed_manifest_exe
    CONFIG -= flat

    RC_FILE = icon.rc
    DEFINES += EDIT_ON_ACTIVATE
}

win32:console {
    LIBS += -lqtmain
}

console {
    DEFINES += NO_MESSAGEHANDLER
}
