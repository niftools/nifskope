###############################
## TARGETS
###############################
# Note: dir or file in build dir cannot be named the same as the target
# e.g. "docs" target will fail if a "docs" folder is in OUT_PWD


###############################
## Docsys
###############################
# Creates NIF docs for NifSkope release
#
# Requirements:
#    Python, jom (or make/mingw32-make)
#
# Usage:
#    jom release-docs
#
# "release-docs" is an alias for:
#    jom -f Makefile.Release docs
#______________________________

docs.target = docs

# Vars
docsys = $$syspath($${PWD}/build/docsys)
indoc = doc$${QMAKE_DIR_SEP}
out = $$syspath($${OUT_PWD})

# Find out if release or debug because DESTDIR is blank
exists($$out/debug/NifSkope.exe):outdoc = $$syspath($${out}/debug/doc)
exists($$out/release/NifSkope.exe):outdoc = $$syspath($${out}/release/doc)

# COMMANDS

docs.commands += $${QMAKE_CHK_DIR_EXISTS} $${outdoc} $${QMAKE_MKDIR} $${outdoc} $$nt
docs.commands += cd $${docsys} $$nt # cd ./build/docsys
docs.commands += python nifxml_doc.py $$nt # invoke python
# Move *.html files out of ./build/docsys/doc
win32:docs.commands += move /Y $${indoc}*.html $${outdoc} $$nt
else:docs.commands += mv -f $${indoc}*.html $${outdoc} $$nt
# Copy CSS and ICO
docs.commands += $${QMAKE_COPY} $${indoc}*.* $${outdoc} $$nt
# Clean up .pyc files so submodule doesn't become "dirty"
docs.commands += $${QMAKE_DEL_FILE} *.pyc $$nt

docs.CONFIG += recursive


###############################
## Doxygen
###############################
# Creates NifSkope API docs
#
# Requirements:
#	 Doxygen (http://www.stack.nl/~dimitri/doxygen/download.html)
#    sed
#     - Windows: http://gnuwin32.sourceforge.net/packages/sed.htm
#    jom (or make/mingw32-make)
#    To automatically extract tags.zip:
#        7-zip (Windows)
#        unzip (Linux)
#
# Usage:
#    jom release-doxygen
#
# "release-doxygen" is an alias for:
#
#    jom -f Makefile.Release doxygen
#______________________________

doxygen.target = doxygen

# Vars
doxyfile = $$syspath($${OUT_PWD}/Doxyfile)
doxyfilein = $$syspath($${PWD}/build/doxygen/Doxyfile.in)

# Paths
qhgen = $$syspath($$[QT_INSTALL_BINS]/qhelpgenerator.exe)
dot = $$syspath(C:/Program Files (x86)/Graphviz2.37/bin) # TODO
_7z = $$get7z()

# Doxyfile.in Replacements

INPUT = $$re_escape($$syspath($${PWD}/src))
OUTPUT = $$re_escape($$syspath($${OUT_PWD}/apidocs))
ROOT = $$re_escape($$syspath($${PWD}))

GENERATE_QHP = NO
exists($$qhgen):GENERATE_QHP = YES

HAVE_DOT = NO
DOT_PATH = ""
exists($$dot) {
	HAVE_DOT = YES
	DOT_PATH = $$re_escape($${dot})
}

TAGS = $$syspath($${PWD}/build/doxygen/tags)
BINS = $$re_escape($$syspath($$[QT_INSTALL_BINS]))

# Find `sed` command
SED = $$getSed()

# Parse Doxyfile.in
!isEmpty(SED) {

!isEmpty(_7z) {
    win32:doxygen.commands += $${_7z} x $${TAGS}$${QMAKE_DIR_SEP}tags.zip \"-o$${TAGS}\" -aoa $$nt
    unix:doxygen.commands += $${_7z} $${TAGS}$${QMAKE_DIR_SEP}tags.zip -o -d $${TAGS} $$nt
}

doxygen.commands += $${SED} -e \"s/@VERSION@/$$getVersion()/g;\
                                 s/@REVISION@/$$getRevision()/g;\
                                 s/@OUTPUT@/$${OUTPUT}/g;\
                                 s/@INPUT@/$${INPUT}/g;\
                                 s/@PWD@/$${ROOT}/g;\
                                 s/@QT_VER@/$$QtHex()/g;\
                                 s/@GENERATE_QHP@/$${GENERATE_QHP}/g;\
                                 s/@HAVE_DOT@/$${HAVE_DOT}/g;\
                                 s/@DOT_PATH@/$${DOT_PATH}/g;\
                                 s/@QT_INSTALL_BINS@/$${BINS}/g\" \
                    $${doxyfilein} > $${doxyfile} $$nt

# Run Doxygen
doxygen.commands += doxygen $${doxyfile} $$nt

} else {

} # end isEmpty

doxygen.CONFIG += recursive


###############################
## ADD TARGETS
###############################

QMAKE_EXTRA_TARGETS += docs doxygen



# Unset Vars

unset(docsys)
unset(indoc)
unset(out)
unset(outdoc)

unset(doxyfilein)
unset(doxyfile)

unset(INPUT)
unset(OUTPUT)
unset(ROOT)
unset(GENERATE_QHP)
unset(HAVE_DOT)
unset(DOT_PATH)
unset(BINS)
unset(TAGS)
unset(_7z)
