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

# On Windows this builds Release in release/ and Debug in debug/
# On Linux you may need CONFIG += debug_and_release debug_and_release_target
DESTDIR = .

HEADERS += \
    *.h \
    gl/*.h \
    gl/marker/*.h \
    gl/dds/*.h \
    widgets/*.h \
    spells/*.h \
    importex/*.h \
    NvTriStrip/qtwrapper.h

SOURCES += \
    *.cpp \
    gl/*.cpp \
    gl/dds/*.cpp \
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
    
    # Ignore specific errors that are very common in the code
    # CFLAGS += /Zc:wchar_t-
    # QMAKE_CFLAGS += /Zc:wchar_t- /wd4305
    # QMAKE_CXXFLAGS += /Zc:forScope- /Zc:wchar_t- /wd4305 
    
    # add specific libraries to msvc builds
    MSVCPROJ_LIBS += winmm.lib Ws2_32.lib imm32.lib
}

win32:console {
    LIBS += -lqtmain
}

console {
    DEFINES += NO_MESSAGEHANDLER
}

TRANSLATIONS += lang/NifSkope_de.ts lang/NifSkope_fr.ts

# vim: set filetype=config : 
