TEMPLATE = app
LANGUAGE = C++
TARGET   = NifSkope

QT += xml opengl network

CONFIG += qt debug_and_release debug_and_release_target thread warn_on

CONFIG += fsengine nvtristrip qhull

INCLUDEPATH += lib

unix:!macx {
                LIBS += -lGLU
}

macx{
        LIBS += -framework CoreFoundation
}

# uncomment this if you want all the messages to be logged to stdout
#CONFIG += console

# uncomment this if you want the text stats gl option
#DEFINES += USE_GL_QPAINTER

DESTDIR = .

# NIFSKOPE_VERSION macro
DEFINES += NIFSKOPE_VERSION=\\\"$$cat(VERSION)\\\"

# build NIFSKOPE_REVISION macro
GIT_HEAD = $$cat(.git/HEAD)
# at this point GIT_HEAD either contains commit hash, or symbolic ref:
# GIT_HEAD = 303c05416ecceb3368997c86676a6e63e968bc9b
# GIT_HEAD = ref: refs/head/feature/blabla
contains(GIT_HEAD, "ref:") {
  # resolve symbolic ref
  GIT_HEAD = .git/$$member(GIT_HEAD, 1)
  # GIT_HEAD now points to the file containing hash,
  # e.g. .git/refs/head/feature/blabla
  exists($$GIT_HEAD) {
    GIT_HEAD = $$cat($$GIT_HEAD)
  } else {
    clear(GIT_HEAD)
  }
}
count(GIT_HEAD, 1) {
  # single component, hopefully the commit hash
  # fetch first seven characters (abbreviated hash)
  GIT_HEAD ~= s/^(.......).*/\\1/
  DEFINES += NIFSKOPE_REVISION=\\\"$$GIT_HEAD\\\"
}

HEADERS += \
    src/basemodel.h \
    src/config.h \
    src/gl/dds/BlockDXT.h \
    src/gl/dds/Color.h \
    src/gl/dds/ColorBlock.h \
    src/gl/dds/Common.h \
    src/gl/dds/dds_api.h \
    src/gl/dds/DirectDrawSurface.h \
    src/gl/dds/Image.h \
    src/gl/dds/PixelFormat.h \
    src/gl/dds/Stream.h \
    src/gl/glcontrolable.h \
    src/gl/glcontroller.h \
    src/gl/GLee.h \
    src/gl/glmarker.h \
    src/gl/glmesh.h \
    src/gl/glnode.h \
    src/gl/glparticles.h \
    src/gl/glproperty.h \
    src/gl/glscene.h \
    src/gl/gltex.h \
    src/gl/gltexloaders.h \
    src/gl/gltools.h \
    src/gl/marker/constraints.h \
    src/gl/marker/furniture.h \
    src/gl/renderer.h \
    src/glview.h \
    src/hacking.h \
    src/importex/3ds.h \
    src/kfmmodel.h \
    src/message.h \
    src/nifexpr.h \
    src/nifitem.h \
    src/nifmodel.h \
    src/nifproxy.h \
    src/nifskope.h \
    src/niftypes.h \
    src/nifvalue.h \
    src/nvtristripwrapper.h \
    src/options.h \
    src/qhull.h \
    src/spellbook.h \
    src/spells/blocks.h \
    src/spells/mesh.h \
    src/spells/misc.h \
    src/spells/skeleton.h \
    src/spells/stringpalette.h \
    src/spells/tangentspace.h \
    src/spells/texture.h \
    src/spells/transform.h \
    src/widgets/colorwheel.h \
    src/widgets/copyfnam.h \
    src/widgets/fileselect.h \
    src/widgets/floatedit.h \
    src/widgets/floatslider.h \
    src/widgets/groupbox.h \
    src/widgets/inspect.h \
    src/widgets/nifcheckboxlist.h \
    src/widgets/nifeditors.h \
    src/widgets/nifview.h \
    src/widgets/refrbrowser.h \
    src/widgets/uvedit.h \
    src/widgets/valueedit.h \
    src/widgets/xmlcheck.h \
    src/ui/about_dialog.h

SOURCES += \
    src/basemodel.cpp \
    src/gl/dds/BlockDXT.cpp \
    src/gl/dds/ColorBlock.cpp \
    src/gl/dds/dds_api.cpp \
    src/gl/dds/DirectDrawSurface.cpp \
    src/gl/dds/Image.cpp \
    src/gl/dds/Stream.cpp \
    src/gl/glcontroller.cpp \
    src/gl/GLee.cpp \
    src/gl/glmarker.cpp \
    src/gl/glmesh.cpp \
    src/gl/glnode.cpp \
    src/gl/glparticles.cpp \
    src/gl/glproperty.cpp \
    src/gl/glscene.cpp \
    src/gl/gltex.cpp \
    src/gl/gltexloaders.cpp \
    src/gl/gltools.cpp \
    src/gl/renderer.cpp \
    src/glview.cpp \
    src/importex/3ds.cpp \
    src/importex/importex.cpp \
    src/importex/obj.cpp \
    src/importex/col.cpp \
    src/kfmmodel.cpp \
    src/kfmxml.cpp \
    src/message.cpp \
    src/nifdelegate.cpp \
    src/nifexpr.cpp \
    src/nifmodel.cpp \
    src/nifproxy.cpp \
    src/nifskope.cpp \
    src/niftypes.cpp \
    src/nifvalue.cpp \
    src/nifxml.cpp \
    src/nvtristripwrapper.cpp \
    src/options.cpp \
    src/qhull.cpp \
    src/spellbook.cpp \
    src/spells/animation.cpp \
    src/spells/blocks.cpp \
    src/spells/bounds.cpp \
    src/spells/color.cpp \
    src/spells/flags.cpp \
    src/spells/fo3only.cpp \
    src/spells/havok.cpp \
    src/spells/headerstring.cpp \
    src/spells/light.cpp \
    src/spells/material.cpp \
    src/spells/mesh.cpp \
    src/spells/misc.cpp \
    src/spells/moppcode.cpp \
    src/spells/morphctrl.cpp \
    src/spells/normals.cpp \
    src/spells/optimize.cpp \
    src/spells/sanitize.cpp \
    src/spells/skeleton.cpp \
    src/spells/stringpalette.cpp \
    src/spells/strippify.cpp \
    src/spells/tangentspace.cpp \
    src/spells/texture.cpp \
    src/spells/transform.cpp \
    src/widgets/colorwheel.cpp \
    src/widgets/copyfnam.cpp \
    src/widgets/fileselect.cpp \
    src/widgets/floatedit.cpp \
    src/widgets/floatslider.cpp \
    src/widgets/groupbox.cpp \
    src/widgets/inspect.cpp \
    src/widgets/nifcheckboxlist.cpp \
    src/widgets/nifeditors.cpp \
    src/widgets/nifview.cpp \
    src/widgets/refrbrowser.cpp \
    src/widgets/uvedit.cpp \
    src/widgets/valueedit.cpp \
    src/widgets/xmlcheck.cpp \
    src/ui/about_dialog.cpp

RESOURCES += \
    res/nifskope.qrc

FORMS += \
	src/ui/about_dialog.ui

fsengine {
    DEFINES += FSENGINE
	INCLUDEPATH += lib/fsengine
    HEADERS += \
        lib/fsengine/bsa.h \
        lib/fsengine/fsengine.h \
        lib/fsengine/fsmanager.h
    SOURCES += \
        lib/fsengine/bsa.cpp \
        lib/fsengine/fsengine.cpp \
        lib/fsengine/fsmanager.cpp
}

nvtristrip {
	INCLUDEPATH += lib/NvTriStrip
	HEADERS += \
		lib/NvTriStrip/NvTriStrip.h \
		lib/NvTriStrip/NvTriStripObjects.h \
		lib/NvTriStrip/VertexCache.h
	SOURCES += \
		lib/NvTriStrip/NvTriStrip.cpp \
		lib/NvTriStrip/NvTriStripObjects.cpp \
		lib/NvTriStrip/VertexCache.cpp
}

qhull {
	INCLUDEPATH += lib/qhull/src/
	HEADERS += \
		lib/qhull/src/libqhull/geom.h \
		lib/qhull/src/libqhull/io.h \
		lib/qhull/src/libqhull/libqhull.h \
		lib/qhull/src/libqhull/mem.h \
		lib/qhull/src/libqhull/merge.h \
		lib/qhull/src/libqhull/poly.h \
		lib/qhull/src/libqhull/qhull_a.h \
		lib/qhull/src/libqhull/qset.h \
		lib/qhull/src/libqhull/random.h \
		lib/qhull/src/libqhull/stat.h \
		lib/qhull/src/libqhull/user.h
}

win32 {
    # useful for MSVC2005
    CONFIG += embed_manifest_exe
    CONFIG -= flat

	RC_FILE = res/icon.rc
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
