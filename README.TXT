==============
== NifSkope ==
==============
 
NifSkope is a tool for analyzing and editing NetImmerse files (.nif)

This is version 1.0.9 of NifSkope.

Known Issues and Workarounds
----------------------------

* On some graphic cards textures show up black or distorted. If this happens,
  uncheck "Shaders" in NifSkope's settings.

* If one of NifSkope's windows disappears due to moving it to a location
  outside of the desktop view, close NifSkope and remove the NifSkope settings
  by deleting the registry key(s)

    HKEY_USERS\S-1-5-***\Software\NifTools\NifSkope-***

  Launch NifSkope again, and all windows will appear at their default
  position.

* If you get a message similar to

    "The application is not configured correctly. Reinstalling it may help."

  or

    "Side by side configuration is incorrect."

  when starting NifSkope, install this Microsoft package which contains Visual
  C++ DLL files:

    http://www.microsoft.com/downloads/details.aspx?familyid=9b2da534-3e03-4391-8a4d-074b9f2bc1bf&displaylang=en

* If NifSkope does not start at all, check your firewall. NifSkope uses the
  network (locally, nothing is sent to the outside world) to communicate with
  itself.

Reporting bugs
--------------

http://sourceforge.net/tracker/?group_id=149157&atid=776343
 
Compiling NifSkope from source
------------------------------

http://niftools.sourceforge.net/wiki/NifSkope/Compile
