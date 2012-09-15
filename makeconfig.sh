#!/bin/bash

version=`cat VERSION`
cat Doxyfile.in | sed 's/@VERSION@/'${version}'/' > Doxyfile
cat README.txt.in | sed 's/@VERSION@/'${version}'/' > README.txt
cat linux-install/nifskope.spec.in | sed 's/@VERSION@/'${version}'/' > linux-install/nifskope.spec
