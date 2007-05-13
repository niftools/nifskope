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


This is version 0.9.8 of NifSkope.


changes since 0.9.7:
  - Bugfix Re-Release
  - added docs by default and fixed RefBrowser bugs
  - changed relative path identifier from "$NIFDIR" to the standard "./"

changes since 0.9.6:
  - texture layout editor
  - change textures via drag&drop
  - support for single texture properties from StarTrek:BridgeCommander
  - import .3ds
  - pack havok strips
  - experimental visuals for stiff spring and ragdoll constraints
  - reference browser

changes since 0.9.5:
  - Fixed bug that would crash NifSkope in FlipUVs
  - Improved stability
  - Copy filename buttons
  - Visual load/save feedback
  - Localization system

changes since 0.9.4:
  - oblivion glow map shaders
  - anisotropic texture filtering (check the antialias checkbox in the render settings dialog)
  - fixed another issue with update tangent space, should look better now
  - the havok strips are now drawn like they appear in the tescs
    if you encounter the spider web effect use the new unstich strips spell on the NiTriStripsData block
  - the skin partition spell now also accepts nifs below 20.x.x.x
  - added an option to save and restore the 3d view's rotation, position and distance
  - rotations can now also be edited as axis angle pairs
  - added a widget style sheet (style.qss)
    if you want you can customize the look and feel of NifSkope through this style sheet
  - fixed an issue with the ipc socket
    NifSkope should open now at least one window regardless of any personal firewall settings

changes since 0.9.3:
  - support for nifs below version 4 (StarTrek: Bridge Commander)
  - comprehensive render settings dialog
  - texture folder auto setup
  - highlighting of individual vertices, normals, triangles, etc.
  - added a spell to combine equal properties and another spell to make the properties unique again
  - improved skin partitioning
  - fixed calculation and rendering of binormals, under some circumstances the direction of the binormals was flipped
    if you encounter this problem updating the tangent space again should solve it
  - fixed flag spell
  - fixed LOD switching
  - fixed Morrowind morph animations
  - template column integrated into type column
  - switched to Qt 4.2


changes since 0.9.2:
  - for enumerated values (eg: havok material) nifskope now displays the option name instead of an integer value
    also the type column's tooltip now displays the type's description and all the available options
  - the version tag syntax in the xml description was slighty changed to be more intuitive
  - clicking in the 3d view now cycles through the items below the mouse


changes since 0.9.1:
  - texture paths are now converted to windows notation
    because otherwise Oblivion doesn't find textures located within .bsa archives
  - stripifier now outputs single strips
    because CivIV draws only the first strip
	

changes since 0.8.8:
  - now plays embedded animation sequences (good examples are the doors and some traps from oblivion)
  - thanks to tapzn for implementing bspline animation interpolators
  - new spell: attach .kf
    this spell takes an external animation sequence and attaches it to the nif
	this way it's now possible to view the creature and character animations in NifSkope
  - fixed havok sorter: now sorts constraints and bhkNiTriStripsShape too
  - updated the skin partitioner: now automatically reduces vertex influences and splits large partitions into smaller parts
  - some small tweaks:
    paste branch now automatically parents ninode branches to the selected ninode
	ipc over udp ensures that new NifSkope windows are allways opened in the same application instance
	   (this makes the 'new window' workaround obsolete when copying and pasting between nifs on windows platform)
	   note that this works by opening a local udp server socket. communication on this socket is
	   limited to local host only. so it shouldn't be a security issue.
 
  
changes since 0.8.7:
  - havok blocks will be ordered automatically
  - some hot keys for the spells
  - fixed obj import/export (textures,collision meshes)
  - new spell allows editing of string offsets
  - new spell for removing blocks by id
  - new spell to upload morph frames
  

changes since 0.8.6:
  - gundalf contributed a new feature: oblivion furniture animation markers
  - Skin Partitions made with NifSkope should now work properly on nvidia cards too
  

changes since 0.8.5:
  - fixed compability with older graphic cards
  - fixed animation controllers
  - fixed triangle sorting
  - fixed an issue with Make Skin Partion on meshes with less than four bones


changes since 0.8.2:
  - two oblivion style shaders displaying normal and parallax mapping
  - automatically reorders the children link arrays for oblivion (trishapes first, ninodes last)
  - new spell: Mesh/Make Skin Partition
  - many cosmetic changes in nif.xml


changes since 0.8.1:
  - multi shape .obj export/import
  - remove branch spell
  - make normals and smooth normals spells
  - added an option to show the havok shapes
  - tool tip descriptions for all blocks and items


changes since 0.8:
  - XML updated
  - create texture templates
  - .OBJ import/export


changes since 0.7:
  - Billboards
  - LOD switching
  - Model walk (walk around the nifs using the wasd key combo)
  - Perspective / Orthographic projection in normal view mode
  - improved vertex color lighting
  - improved boundary scan
  - hide non textured meshes (daoc compability)
  - hide nodes by name
  - new debug spell: thread load


changes since 0.6:
  - lights
  - particles (80% of the old particle systems from morrowind)
  - xml: template types + optimized conditions
  - fixed texturing again:
    - NIF folder is now searched for textures too
    - smart cache flush control (reloads textures if needed)
    - fixed RGBA32 TGAs which have their alphaBitCnt set to zero
    - fixed TGA RLE decompression (was much too slow)
    - added support for TGA L8 and LA16 (greyscale)
    - fixed bit stuffed DDS formats (RGBA 5551 565 4444 ...)
    - alternative extension test (nifs from mournhold and solstheim)
    - fixed tex folder deselect
    - fixed filenames containing \r\n\0

changes since 0.5:
  - new flexible context menu system (spells)
  - texture spells ( choose + pack + export + add glow/base map )
  - strippify spell converts NiTriShape to NiTriStrip
  - gl view now uses filtered mipmaps
  - various fixes and adjustments

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



Recompiling NifSkope from source:

1) download and install the Qt4 sources from http://www.trolltech.com/download/qt
2) download the nifskope source via SVN from https://svn.sourceforge.net/svnroot/niftools/trunk/nifskope
3) edit nifskope.pro to suit your needs
4) start QT Command Prompt (windows) or open up a shell (linux)
5) cd to nifskope's directory
6) qmake
7) make
8) nifskope

