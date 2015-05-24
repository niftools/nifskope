###############################
## BUILD OPTIONS
###############################

TEMPLATE = app
TARGET   = NifSkope

QT += xml opengl network widgets

# Require Qt 5.3 or higher
contains(QT_VERSION, ^5\\.[0-2]\\..*) {
	message("Cannot build NifSkope with Qt version $${QT_VERSION}")
	error("Minimum required version is Qt 5.3")
}

# C++11 Support
CONFIG += c++11

# Dependencies
CONFIG += nvtristrip qhull soil
win32:CONFIG += fsengine

# Debug/Release options
CONFIG(debug, debug|release) {
	# Debug Options
	BUILD = debug
	CONFIG += console
} else {
	# Release Options
	BUILD = release
	CONFIG -= console
	DEFINES += QT_NO_DEBUG_OUTPUT
}
# TODO: Get rid of this define
#	uncomment this if you want the text stats gl option
#	DEFINES += USE_GL_QPAINTER

INCLUDEPATH += src lib

TRANSLATIONS += \
	res/lang/NifSkope_de.ts \
	res/lang/NifSkope_fr.ts

# Require explicit
DEFINES += \
	QT_NO_CAST_FROM_BYTEARRAY \ # QByteArray deprecations
	QT_NO_URL_CAST_FROM_STRING \ # QUrl deprecations
	QT_DISABLE_DEPRECATED_BEFORE=0x050300 #\ # Disable all functions deprecated as of 5.3

	# Useful for tracking down strings not using
	#	QObject::tr() for translations.
	# QT_NO_CAST_FROM_ASCII \
	# QT_NO_CAST_TO_ASCII


VISUALSTUDIO = false
*msvc* {
	######################################
	## Detect Visual Studio vs Qt Creator
	######################################
	#	Qt Creator = shadow build
	#	Visual Studio = no shadow build

	# Strips PWD (source) from OUT_PWD (build) to test if they are on the same path
	#	- contains() does not work
	#	- equals( PWD, $${OUT_PWD} ) is not sufficient
	REP = $$replace(OUT_PWD, $${PWD}, "")

	# Test if Build dir is outside Source dir
	#	if REP == OUT_PWD, not Visual Studio
	!equals( REP, $${OUT_PWD} ):VISUALSTUDIO = true
	unset(REP)

	# Set OUT_PWD to ./bin so that qmake doesn't clutter PWD
	#	Unfortunately w/ VS qmake still creates empty debug/release folders in PWD.
	#	They are never used but get auto-generated because of CONFIG += debug_and_release
	$$VISUALSTUDIO:OUT_PWD = $${_PRO_FILE_PWD_}/bin
}

###############################
## FUNCTIONS
###############################

include(NifSkope_functions.pri)


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

# build_pass is necessary
# Otherwise it will create empty .moc, .ui, etc. dirs on the drive root
build_pass|!debug_and_release {
	win32:equals( VISUALSTUDIO, true ) {
		# Visual Studio
		DESTDIR = $${_PRO_FILE_PWD_}/bin/$${BUILD}
		# INTERMEDIATE FILES
		INTERMEDIATE = $${DESTDIR}/../GeneratedFiles/$${BUILD}
	} else {
		# Qt Creator
		DESTDIR = $${OUT_PWD}/$${BUILD}
		# INTERMEDIATE FILES
		INTERMEDIATE = $${DESTDIR}/../GeneratedFiles/
	}

	UI_DIR = $${INTERMEDIATE}/.ui
	MOC_DIR = $${INTERMEDIATE}/.moc
	RCC_DIR = $${INTERMEDIATE}/.qrc
	OBJECTS_DIR = $${INTERMEDIATE}/.obj
}

###############################
## TARGETS
###############################

include(NifSkope_targets.pri)


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
	src/gl/controllers.h \
	src/gl/glcontroller.h \
	src/gl/glmarker.h \
	src/gl/glmesh.h \
	src/gl/glnode.h \
	src/gl/glparticles.h \
	src/gl/glproperty.h \
	src/gl/glscene.h \
	src/gl/gltex.h \
	src/gl/gltexloaders.h \
	src/gl/gltools.h \
	src/gl/icontrollable.h \
	src/gl/marker/constraints.h \
	src/gl/marker/furniture.h \
	src/gl/renderer.h \
	src/glview.h \
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
	src/settings.h \
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
	src/ui/about_dialog.h \
	src/ui/checkablemessagebox.h \
	src/ui/settingsdialog.h \
	src/version.h

SOURCES += \
	src/basemodel.cpp \
	src/gl/dds/BlockDXT.cpp \
	src/gl/dds/ColorBlock.cpp \
	src/gl/dds/dds_api.cpp \
	src/gl/dds/DirectDrawSurface.cpp \
	src/gl/dds/Image.cpp \
	src/gl/dds/Stream.cpp \
	src/gl/controllers.cpp \
	src/gl/glcontroller.cpp \
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
	src/nifskope_ui.cpp \
	src/niftypes.cpp \
	src/nifvalue.cpp \
	src/nifxml.cpp \
	src/nvtristripwrapper.cpp \
	src/options.cpp \
	src/qhull.cpp \
	src/settings.cpp \
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
	src/ui/about_dialog.cpp \
	src/ui/checkablemessagebox.cpp \
	src/ui/settingsdialog.cpp \
	src/version.cpp

RESOURCES += \
	res/nifskope.qrc

FORMS += \
	src/ui/about_dialog.ui \
	src/ui/checkablemessagebox.ui \
	src/ui/nifskope.ui \
	src/ui/settingsdialog.ui \
	src/ui/settingsgeneral.ui


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

soil {
    INCLUDEPATH += lib/soil
    HEADERS += \
        lib/soil/image_DXT.h \
        lib/soil/image_helper.h \
        lib/soil/SOIL.h \
        lib/soil/stb_image_aug.h \
        lib/soil/stbi_DDS_aug.h \
        lib/soil/stbi_DDS_aug_c.h
    SOURCES += \
        lib/soil/image_DXT.c \
        lib/soil/image_helper.c \
        lib/soil/SOIL.c \
        lib/soil/stb_image_aug.c
}


###############################
## COMPILER SCOPES
###############################

QMAKE_CXXFLAGS_RELEASE -= -O
QMAKE_CXXFLAGS_RELEASE -= -O1
QMAKE_CXXFLAGS_RELEASE -= -O2

win32 {
	RC_FILE = res/icon.rc
	DEFINES += EDIT_ON_ACTIVATE
}

# MSVC
#  Both Visual Studio and Qt Creator
#  Required: msvc2013 or higher
*msvc* {

	# Grab _MSC_VER from the mkspecs that Qt was compiled with
	#	e.g. VS2013 = 1800, VS2012 = 1700, VS2010 = 1600
	_MSC_VER = $$find(QMAKE_COMPILER_DEFINES, "_MSC_VER")
	_MSC_VER = $$split(_MSC_VER, =)
	_MSC_VER = $$member(_MSC_VER, 1)

	# Reject unsupported MSVC versions
	!isEmpty(_MSC_VER):lessThan(_MSC_VER, 1800) {
		error("NifSkope only supports MSVC 2013 or later. If this is too prohibitive you may use Qt Creator with MinGW.")
	}

	# So VCProj Filters do not flatten headers/source
	CONFIG -= flat

	# COMPILER FLAGS

	#  Optimization flags
	QMAKE_CXXFLAGS_RELEASE *= -O2 -arch:SSE2 # SSE2 is the default, but make it explicit
	#  Multithreaded compiling for Visual Studio
	QMAKE_CXXFLAGS += -MP

	# LINKER FLAGS

	#  Relocate .lib and .exp files to keep release dir clean
	QMAKE_LFLAGS += /IMPLIB:$$syspath($${INTERMEDIATE}/NifSkope.lib)

	#  PDB location
	QMAKE_LFLAGS_DEBUG += /PDB:$$syspath($${INTERMEDIATE}/nifskope.pdb)

	#  Clean up .embed.manifest from release dir
	#	Fallback for `Manifest Embed` above
	QMAKE_POST_LINK += $$QMAKE_DEL_FILE $$syspath($${DESTDIR}/*.manifest) $$nt
}


# MinGW, GCC
#  Recommended: GCC 4.8.1+
*-g++ {

	# COMPILER FLAGS

	#  Optimization flags
	QMAKE_CXXFLAGS_DEBUG -= -O0 -g
	QMAKE_CXXFLAGS_DEBUG *= -Og -g3
	QMAKE_CXXFLAGS_RELEASE *= -O3 -mfpmath=sse

	# C++11 Support
	QMAKE_CXXFLAGS_RELEASE *= -std=c++11

	#  Extension flags
	QMAKE_CXXFLAGS_RELEASE *= -msse2 -msse
}

unix:!macx {
	LIBS += -lGLU
}

macx {
	LIBS += -framework CoreFoundation
}


# Pre/Post Link in build_pass only
build_pass|!debug_and_release {

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

	win32:DEP += \
		dep/NifMopp.dll

	XML += \
		build/docsys/nifxml/nif.xml \
		build/docsys/kfmxml/kfm.xml

	QSS += \
		res/style.qss

	QHULLTXT += \
		lib/qhull/COPYING.txt

	#LANG += \
	#	res/lang

	SHADERS += \
		res/shaders

	READMES += \
		CHANGELOG.md \
		CONTRIBUTORS.md \
		LICENSE.md \
		README.md \
		TROUBLESHOOTING.md


	copyDirs( $$SHADERS, shaders )
	#copyDirs( $$LANG, lang )
	copyFiles( $$XML $$QSS )
	win32:copyFiles( $$DEP )

	# Copy Readmes and rename to TXT
	copyFiles( $$READMES,,,, md:txt )

	# Copy Qhull COPYING.TXT and rename
	copyFiles( $$QHULLTXT,,, Qhull_COPYING.txt )

	win32:!static {
		# Copy DLLs to build dir
		copyFiles( $$QtBins(),, true )

		platforms += \
			$$[QT_INSTALL_PLUGINS]/platforms/qminimal$${DLLEXT} \
			$$[QT_INSTALL_PLUGINS]/platforms/qwindows$${DLLEXT}
		
		imageformats += \
			$$[QT_INSTALL_PLUGINS]/imageformats/qdds$${DLLEXT} \
			$$[QT_INSTALL_PLUGINS]/imageformats/qjpeg$${DLLEXT} \
			$$[QT_INSTALL_PLUGINS]/imageformats/qtga$${DLLEXT} \
			$$[QT_INSTALL_PLUGINS]/imageformats/qwebp$${DLLEXT}

		copyFiles( $$platforms, platforms, true )
		copyFiles( $$imageformats, imageformats, true )
	}

} # end build_pass


# Build Messages
# (Add `buildMessages` to CONFIG to use)
buildMessages:build_pass|buildMessages:!debug_and_release {
	CONFIG(debug, debug|release) {
		message("Debug Mode")
	} CONFIG(release, release|debug) {
		message("Release Mode")
	}

	message(mkspec _______ $$QMAKESPEC)
	message(cxxflags _____ $$QMAKE_CXXFLAGS)
	message(arch _________ $$QMAKE_TARGET.arch)
	message(src __________ $$PWD)
	message(build ________ $$OUT_PWD)
	message(Qt binaries __ $$[QT_INSTALL_BINS])

	build_pass:equals( VISUALSTUDIO, true ) {
		message(Visual Studio __ Yes)
	}

	#message($$CONFIG)
}

# vim: set filetype=config : 
