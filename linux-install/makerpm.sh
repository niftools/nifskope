#!/bin/sh

# script for preparing tarball

VERSION=1.0.1

FILES="NifSkope.pro \
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
    gl/*.h \
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
    widgets/*.cpp \
    spells/*.cpp \
    importex/*.cpp \
    NvTriStrip/*.cpp \
    fsengine/*.h \
    fsengine/*.cpp"

# clean old tarball
rm -rf nifskope-$VERSION
rm -f ~/rpmbuild/SOURCES/nifskope-$VERSION.tar.bz2

# create tarball
mkdir nifskope-$VERSION
cd nifskope-$VERSION
mkdir -p gl widgets NvTriStrip spells importex fsengine
cd ../..
cp --parents $FILES linux-install/nifskope-$VERSION
cd linux-install
tar cfvj ~/rpmbuild/SOURCES/nifskope-$VERSION.tar.bz2 nifskope-$VERSION

# clean
rm -rf nifskope-$VERSION

