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
    qhull/src/libqhull/qhull_a.h
    qhull/src/libqhull/libqhull.c
    qhull/src/libqhull/mem.c
    qhull/src/libqhull/qset.c
    qhull/src/libqhull/geom.c
    qhull/src/libqhull/merge.c
    qhull/src/libqhull/poly.c
    qhull/src/libqhull/io.c
    qhull/src/libqhull/stat.c
    qhull/src/libqhull/global.c
    qhull/src/libqhull/user.c
    qhull/src/libqhull/poly2.c
    qhull/src/libqhull/geom2.c
    qhull/src/libqhull/userprintf.c
    qhull/src/libqhull/usermem.c
    qhull/src/libqhull/random.c
    qhull/src/libqhull/rboxlib.c
    qhull/src/libqhull/libqhull.h
    qhull/src/libqhull/stat.h
    qhull/src/libqhull/random.h
    qhull/src/libqhull/mem.h
    qhull/src/libqhull/geom.h
    qhull/src/libqhull/merge.h
    qhull/src/libqhull/poly.h
    qhull/src/libqhull/io.h
    qhull/src/libqhull/user.h
    qhull/src/libqhull/qset.h
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

