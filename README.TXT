***** BEGIN LICENSE BLOCK *****

BSD License

Copyright (c) 2005, NIF File Format Library and Tools
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the NIF File Format Library and Tools projectmay not be
   used to endorse or promote products derived from this software
   without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

***** END LICENCE BLOCK *****



  NifSkope
 ==========
 
NifSkope is a tool for analyzing and editing NetImmerse files (.nif)


This is version 0.5 of NifSkope.

It should read and write every Nif up to version 10.2.0.0.


changes since 0.4:
  - more animation controllers: flip, morph, vis, uv
  - NiPixelData textures are now displayed too
  - open gl 3d view is now in sync with the nif model:
      changes to the data structure show up immediately
      (no more double click to update)
  - some internal changes:
      NifValue replaces QVariant
      NifValue uses hardcoded streaming class

changes since 0.3:

  - switched to Qt 4.1
  - new main window layout
  - multiple main windows
  - copy/paste blocks and compounds
  - draw bones/nodes
  - keyframe animations
  - draw hidden meshes
  - nif version check on load
  - fixed various texture issues
  - vector, matrix, quat are now internal types


changes since 0.2:

  - opengl view rewritten:
  - fixed display of better body meshes
  - fixed model center on load
  - improved alpha blending
  - improved material colors and lighting
  - fixed performance issue with textures on cdrom

  - application icon by Shon

  - changed source code license to BSD


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
