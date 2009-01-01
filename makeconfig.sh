#!/bin/sh

wcrev=`svn info | sed -n '/^Last Changed Rev/ s/[^0-9]//gp'`
sed 's/\$WCREV\$/'${wcrev}'/' config.h.in > config.h

