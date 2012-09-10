#!/bin/bash

version=`cat VERSION`
wcrev=`git log -1 --pretty=format:%h`
cat config.h.in | sed 's/@WCREV@/'${wcrev}'/' | sed 's/@VERSION@/'${version}'/' > config.h
cat Doxyfile.in | sed 's/@WCREV@/'${wcrev}'/' | sed 's/@VERSION@/'${version}'/' > Doxyfile
cat README.txt.in | sed 's/@WCREV@/'${wcrev}'/' | sed 's/@VERSION@/'${version}'/' > README.txt
cat linux-install/nifskope.spec.in | sed 's/@WCREV@/'${wcrev}'/' | sed 's/@VERSION@/'${version}'/' > linux-install/nifskope.spec
