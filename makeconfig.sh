#!/bin/sh

wcrev=`svn info | sed -n '/^Last Changed Rev/ s/[^0-9]//gp'`
grep ${wcrev} config.h > /dev/null
if [[ $? == 0 ]]; then
    exit
else
    sed 's/\$WCREV\$/'${wcrev}'/' config.h.in > config.h
fi

