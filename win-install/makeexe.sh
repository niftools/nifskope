NAME=nifskope
VERSION=`cat ../VERSION`
wcrev=`git log -1 --pretty=format:%h`
if [ "$1" == "" ]
then
    extversion=${VERSION}.${wcrev}
else
    extversion=${VERSION}-$1.${wcrev}
fi

rm nifskope-*.exe
echo !define VERSION \"${VERSION}\" > nifversion.nsh
echo !define BUILD_RELEASE_FOLDER \"../release\" >> nifversion.nsh
echo !define DLL_RELEASE_FOLDER \"/usr/i686-w64-mingw32/sys-root/mingw/bin\" >> nifversion.nsh

cd ../docsys
rm doc/*.html
python nifxml_doc.py

pushd ../lang
find *.ts -exec lrelease-qt4 {} \;
popd

cp ../qhull/COPYING.txt ../Qhull_COPYING.TXT

cd ../win-install

makensis -V3 ${NAME}-fedora-mingw-dynamic.nsi
mv "${NAME}-${VERSION}-windows.exe" "${NAME}-${extversion}-windows.exe"

