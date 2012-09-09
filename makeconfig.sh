#!/bin/bash

version=`cat VERSION`

wcrev=`git log -1 --pretty=format:%h`
if [[ -e config.h ]]; then
    grep ${wcrev} config.h > /dev/null
    if [[ $? == 0 ]]; then
        exit
    fi
fi
sed 's/\$WCREV\$/'${wcrev}'/' config.h.in > config.h
sed 's/\$VERSION\$/'${wcrev}'/' config.h.in > config.h

