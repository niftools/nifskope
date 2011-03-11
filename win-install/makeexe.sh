NAME=nifskope
VERSION=1.1.0.`git log -1 --pretty=format:%h`

rm ${NAME}-${VERSION}-windows.exe
echo !define VERSION \"${VERSION}\" > nifversion.nsh
echo !define BUILD_RELEASE_FOLDER \"../release\" >> nifversion.nsh
echo !define DLL_RELEASE_FOLDER \"/usr/i686-pc-mingw32/sys-root/mingw/bin\" >> nifversion.nsh

cd ../docsys
rm doc/*.html
python nifxml_doc.py

pushd ../lang
find *.ts -exec lrelease-qt4 {} \;
popd

cp ../qhull/COPYING.txt ../Qhull_COPYING.TXT

cd ../win-install

makensis -V3 ${NAME}-mingw-dynamic.nsi
