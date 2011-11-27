#!/bin/sh

# Script for preparing the ~/rpmbuild/SOURCES/nifskope-$VERSION.tar.bz2 tarball

# How to build the rpm:

# su -c 'yum install rpmdevtools'
# rpmdev-setuptree
# ./maketarball.sh
# rpmbuild -ba nifskope.spec

# and the rpm will reside in ~/rpmbuild/RPMS

VERSION=1.1.1

FILES="NifSkope.pro \
    TODO.TXT \
    README.TXT \
    CHANGELOG.TXT \
    LICENSE.TXT \
    style.qss \
    nifskope.qrc \
    resources/*.png \
    nifskope.png \
    spells/skel.dat \
    nifitem.h \
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
    config.h \
    gl/*.h \
    gl/dds/*.h \
    gl/marker/*.h \
    widgets/*.h \
    spells/*.h \
    importex/*.h \
    NvTriStrip/*.h \
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
    gl/dds/*.cpp \
    widgets/*.cpp \
    spells/*.cpp \
    importex/*.cpp \
    NvTriStrip/*.cpp \
    fsengine/*.h \
    fsengine/*.cpp
    shaders/*.frag
    shaders/*.prog
    shaders/*.vert
    nifexpr.cpp
    nifexpr.h
    lang/*.ts
    lang/*.qm
    qhull.h
    qhull.cpp
    qhull/COPYING.txt
    qhull/src/qhull_a.h
    qhull/src/libqhull.c
    qhull/src/mem.c
    qhull/src/qset.c
    qhull/src/geom.c
    qhull/src/merge.c
    qhull/src/poly.c
    qhull/src/io.c
    qhull/src/stat.c
    qhull/src/global.c
    qhull/src/user.c
    qhull/src/poly2.c
    qhull/src/geom2.c
    qhull/src/userprintf.c
    qhull/src/usermem.c
    qhull/src/random.c
    qhull/src/rboxlib.c
    qhull/src/libqhull.h
    qhull/src/stat.h
    qhull/src/random.h
    qhull/src/mem.h
    qhull/src/geom.h
    qhull/src/merge.h
    qhull/src/poly.h
    qhull/src/io.h
    qhull/src/user.h
    qhull/src/qset.h
    resources/qhull_cone.gif"

# clean old tarball
rm -rf nifskope-$VERSION
rm -f ~/rpmbuild/SOURCES/nifskope-$VERSION.tar.bz2

# create fresh source directory
mkdir nifskope-$VERSION

# copy xml files
cd ../docsys
cp nifxml/nif.xml kfmxml/kfm.xml ../linux-install/nifskope-$VERSION
cd ../linux-install

# copy docsys files
cd ../docsys
rm -f doc/*.html
python nifxml_doc.py
cp --parents doc/*.html doc/docsys.css doc/favicon.ico ../linux-install/nifskope-$VERSION
cd ../linux-install

# run config script
cd ..
./makeconfig.sh
cd linux-install

# run language scripts
cd ../lang
for i in *.ts; do lrelease-qt4 $i; done
cd ../linux-install

# copy source files
cd nifskope-$VERSION
mkdir -p gl widgets NvTriStrip spells importex fsengine
cd ../..
cp --parents $FILES linux-install/nifskope-$VERSION
cd linux-install

# copy desktop file
cp nifskope.desktop nifskope-$VERSION

# create tarball
tar cfvj ~/rpmbuild/SOURCES/nifskope-$VERSION.tar.bz2 nifskope-$VERSION

# clean
rm -rf nifskope-$VERSION

