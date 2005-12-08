/****************************************************************************
**
** nifskope: a tool for analyzing and editing NetImmerse files (.nif & .kf)
**
** This file may be used under the terms of the GNU General Public
** License version 2.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of
** this file.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/



  NifSkope
 ==========
 
NifSkope is a tool for analyzing and editing NetImmerse files (.nif .kf)


This is version 0.2 of NifSkope.

It should read and write every Nif up to version 10.x.x.x.


changes since 0.1:

  - display block hirarchy

  - display abstract ancestors inline
  - display block names at top level
  - display link target name/type
  - follow link (available through context menu)
  - multi lined strings fixed (line breaks converted to and from "\r\n")

  - fixed compressed tga textures (made with the gimp)
  - proper display of skinned meshes
  - center model on load


If you want to compile NifSkope from source:

1) install the Qt4 sources from http://www.trolltech.com/download/qt (if you don't have them yet)
2) download the nifskope source from http://niftools.sourceforge.net
3) edit nifskope.pro to suit your needs
4) start QT Command Prompt (windows) or open up a shell (linux)
5) cd to nifskope's directory
6) qmake
7) make
8) nifskope
