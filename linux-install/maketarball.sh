#!/bin/sh

# Script for preparing the ~/rpmbuild/SOURCES/nifskope-$VERSION.tar.bz2 tarball

# How to build the rpm:

# yum install rpmdevtools
# rpmdev-setuptree
# ./maketarball.sh
# rpmbuild -ba nifskope.spec

# and the rpm will reside in ~/rpmbuild/RPMS

VERSION=1.0.14

FILES="NifSkope.pro \
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
    shaders/*.vert"

# clean old tarball
rm -rf nifskope-$VERSION
rm -f ~/rpmbuild/SOURCES/nifskope-$VERSION.tar.bz2

# create fresh source directory
mkdir nifskope-$VERSION

# copy xml files
cd ../../docsys
cp nif.xml kfm.xml ../nifskope/linux-install/nifskope-$VERSION
cd ../nifskope/linux-install

# copy docsys files
cd ../../docsys
rm -f doc/*.html
python nifxml_doc.py
cp --parents doc/*.html doc/docsys.css doc/favicon.ico ../nifskope/linux-install/nifskope-$VERSION
cd ../nifskope/linux-install

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

