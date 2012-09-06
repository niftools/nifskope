TEMPLATE = app
LANGUAGE = C++
TARGET   = NifSkope

QT += xml opengl network

CONFIG += qt release thread warn_on

CONFIG += fsengine

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

# On Windows this builds Release in release/ and Debug in debug/
# On Linux you may need CONFIG += debug_and_release debug_and_release_target
DESTDIR = .

HEADERS += \
    basemodel.h \
    config.h \
    gl/dds/BlockDXT.h \
    gl/dds/Color.h \
    gl/dds/ColorBlock.h \
    gl/dds/Common.h \
    gl/dds/dds_api.h \
    gl/dds/DirectDrawSurface.h \
    gl/dds/Image.h \
    gl/dds/PixelFormat.h \
    gl/dds/Stream.h \
    gl/glcontrolable.h \
    gl/glcontroller.h \
    gl/GLee.h \
    gl/glmarker.h \
    gl/glmesh.h \
    gl/glnode.h \
    gl/glparticles.h \
    gl/glproperty.h \
    gl/glscene.h \
    gl/gltex.h \
    gl/gltexloaders.h \
    gl/gltools.h \
    gl/marker/constraints.h \
    gl/marker/furniture.h \
    gl/renderer.h \
    glview.h \
    hacking.h \
    importex/3ds.h \
    kfmmodel.h \
    message.h \
    nifexpr.h \
    nifitem.h \
    nifmodel.h \
    nifproxy.h \
    nifskope.h \
    niftypes.h \
    nifvalue.h \
    NvTriStrip/NvTriStrip.h \
    NvTriStrip/NvTriStripObjects.h \
    NvTriStrip/qtwrapper.h \
    NvTriStrip/VertexCache.h \
    options.h \
    qhull/src/libqhull/geom.h \
    qhull/src/libqhull/io.h \
    qhull/src/libqhull/libqhull.h \
    qhull/src/libqhull/mem.h \
    qhull/src/libqhull/merge.h \
    qhull/src/libqhull/poly.h \
    qhull/src/libqhull/qhull_a.h \
    qhull/src/libqhull/qset.h \
    qhull/src/libqhull/random.h \
    qhull/src/libqhull/stat.h \
    qhull/src/libqhull/user.h \
    qhull.h \
    spellbook.h \
    spells/blocks.h \
    spells/mesh.h \
    spells/misc.h \
    spells/skeleton.h \
    spells/stringpalette.h \
    spells/tangentspace.h \
    spells/texture.h \
    spells/transform.h \
    widgets/colorwheel.h \
    widgets/copyfnam.h \
    widgets/fileselect.h \
    widgets/floatedit.h \
    widgets/floatslider.h \
    widgets/groupbox.h \
    widgets/inspect.h \
    widgets/nifcheckboxlist.h \
    widgets/nifeditors.h \
    widgets/nifview.h \
    widgets/refrbrowser.h \
    widgets/uvedit.h \
    widgets/valueedit.h \
    widgets/xmlcheck.h

SOURCES += \
    basemodel.cpp \
    gl/dds/BlockDXT.cpp \
    gl/dds/ColorBlock.cpp \
    gl/dds/dds_api.cpp \
    gl/dds/DirectDrawSurface.cpp \
    gl/dds/Image.cpp \
    gl/dds/Stream.cpp \
    gl/glcontroller.cpp \
    gl/GLee.cpp \
    gl/glmarker.cpp \
    gl/glmesh.cpp \
    gl/glnode.cpp \
    gl/glparticles.cpp \
    gl/glproperty.cpp \
    gl/glscene.cpp \
    gl/gltex.cpp \
    gl/gltexloaders.cpp \
    gl/gltools.cpp \
    gl/renderer.cpp \
    glview.cpp \
    importex/3ds.cpp \
    importex/importex.cpp \
    importex/obj.cpp \
    kfmmodel.cpp \
    kfmxml.cpp \
    message.cpp \
    nifdelegate.cpp \
    nifexpr.cpp \
    nifmodel.cpp \
    nifproxy.cpp \
    nifskope.cpp \
    niftypes.cpp \
    nifvalue.cpp \
    nifxml.cpp \
    NvTriStrip/NvTriStrip.cpp \
    NvTriStrip/NvTriStripObjects.cpp \
    NvTriStrip/qtwrapper.cpp \
    NvTriStrip/VertexCache.cpp \
    options.cpp \
    qhull.cpp \
    spellbook.cpp \
    spells/animation.cpp \
    spells/blocks.cpp \
    spells/bounds.cpp \
    spells/color.cpp \
    spells/flags.cpp \
    spells/fo3only.cpp \
    spells/havok.cpp \
    spells/headerstring.cpp \
    spells/light.cpp \
    spells/material.cpp \
    spells/mesh.cpp \
    spells/misc.cpp \
    spells/moppcode.cpp \
    spells/morphctrl.cpp \
    spells/normals.cpp \
    spells/optimize.cpp \
    spells/sanitize.cpp \
    spells/skeleton.cpp \
    spells/stringpalette.cpp \
    spells/strippify.cpp \
    spells/tangentspace.cpp \
    spells/texture.cpp \
    spells/transform.cpp \
    widgets/colorwheel.cpp \
    widgets/copyfnam.cpp \
    widgets/fileselect.cpp \
    widgets/floatedit.cpp \
    widgets/floatslider.cpp \
    widgets/groupbox.cpp \
    widgets/inspect.cpp \
    widgets/nifcheckboxlist.cpp \
    widgets/nifeditors.cpp \
    widgets/nifview.cpp \
    widgets/refrbrowser.cpp \
    widgets/uvedit.cpp \
    widgets/valueedit.cpp \
    widgets/xmlcheck.cpp

RESOURCES += \
    nifskope.qrc

fsengine {
    DEFINES += FSENGINE
    HEADERS += \
        fsengine/bsa.h \
        fsengine/fsengine.h \
        fsengine/fsmanager.h
    SOURCES += \
        fsengine/bsa.cpp \
        fsengine/fsengine.cpp \
        fsengine/fsmanager.cpp
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
