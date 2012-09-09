#!/bin/bash

version=`cat VERSION`
wcrev=`git log -1 --pretty=format:%h`
cat config.h.in | sed 's/@WCREV@/'${wcrev}'/' | sed 's/@VERSION@/'${version}'/' > config.h
cat Doxyfile.in | sed 's/@WCREV@/'${wcrev}'/' | sed 's/@VERSION@/'${version}'/' > Doxyfile

