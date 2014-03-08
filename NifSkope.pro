###############################
## BUILD OPTIONS
###############################

TEMPLATE = app
TARGET   = NifSkope

QT += xml opengl network
greaterThan(QT_MAJOR_VERSION, 4) {
	QT += widgets
}

CONFIG -= thread_off
CONFIG *= thread

# Dependencies
CONFIG += fsengine nvtristrip qhull

# Debug/Release options
CONFIG(debug, debug|release) {
	# Debug Options
	CONFIG += console
	# Exits after qWarning() for easy backtrace
	#DEFINES += QT_FATAL_WARNINGS
} else {
	# Release Options
	CONFIG -= console
	DEFINES += QT_NO_DEBUG_OUPUT
	# TODO: Clean up qWarnings first before using
	#DEFINES += QT_NO_WARNING_OUTPUT
}
# TODO: Get rid of this define
#	uncomment this if you want the text stats gl option
#	DEFINES += USE_GL_QPAINTER

INCLUDEPATH += lib .

TRANSLATIONS += \
	lang/NifSkope_de.ts \
	lang/NifSkope_fr.ts


###############################
## INCLUDES
###############################

include(NifSkope_functions.pri)
include(NifSkope_targets.pri)


###############################
## MACROS
###############################

# NifSkope Version
VER = $$getVersion()
# NifSkope Revision
REVISION = $$getRevision()

# NIFSKOPE_VERSION macro
DEFINES += NIFSKOPE_VERSION=\\\"$${VER}\\\"

# NIFSKOPE_REVISION macro
!isEmpty(REVISION) {
	DEFINES += NIFSKOPE_REVISION=\\\"$${REVISION}\\\"
}


###############################
## OUTPUT DIRECTORIES
###############################

build_pass {
	# build_pass is necessary
	# Otherwise it will create empty .moc, .ui, etc. dirs on the drive root
	Debug:   DESTDIR = $${OUT_PWD}/debug
	Release: DESTDIR = $${OUT_PWD}/release

	# INTERMEDIATE FILES
	UI_DIR = $${DESTDIR}/../.ui
	MOC_DIR = $${DESTDIR}/../.moc
	RCC_DIR = $${DESTDIR}/../.qrc
	OBJECTS_DIR = $${DESTDIR}/../.obj
}


###############################
## PROJECT SCOPES
###############################

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


###############################
## DEPENDENCY SCOPES
###############################

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
	INCLUDEPATH += lib/qhull/src
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


###############################
## COMPILER SCOPES
###############################

win32 {
	RC_FILE = res/icon.rc
	DEFINES += EDIT_ON_ACTIVATE
}

*msvc* {
	CONFIG *= embed_manifest_exe
	CONFIG -= flat
}

unix:!macx {
	LIBS += -lGLU
}

macx {
	LIBS += -framework CoreFoundation
}


# Pre/Post Link in build_pass only
build_pass {

###############################
## QMAKE_PRE_LINK
###############################

	# Find `sed` command
	SED = $$getSed()

	!isEmpty(SED) {
		# Replace @VERSION@ with number from build/VERSION
		# Copy build/README.md.in > README.md
		QMAKE_PRE_LINK += $${SED} -e s/@VERSION@/$${VER}/ $${PWD}/build/README.md.in > $${PWD}/README.md $$nt
	}


###############################
## QMAKE_POST_LINK
###############################

	DEP += \
		dep/NifMopp.dll

	XML += \
		build/docsys/nifxml/nif.xml \
		build/docsys/kfmxml/kfm.xml

	QSS += \
		res/style.qss

	QHULLTXT += \
		lib/qhull/COPYING.txt

	LANG += \
		res/lang

	SHADERS += \
		res/shaders

	READMES += \
		CHANGELOG.md \
		CONTRIBUTORS.md \
		LICENSE.md \
		README.md \
		TROUBLESHOOTING.md


	copyDirs( $$SHADERS, shaders )
	copyDirs( $$LANG, lang )
	copyFiles( $$XML $$DEP $$QSS )

	# Copy Readmes and rename to TXT
	copyFiles( $$READMES,,,, md:txt )

	# Copy Qhull COPYING.TXT and rename
	copyFiles( $$QHULLTXT,,, Qhull_COPYING.txt )

	win32:!static {
		# Copy DLLs to build dir
		copyFiles( $$QtBins(),, true )
	}

} # end build_pass

# vim: set filetype=config : 
