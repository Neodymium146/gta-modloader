Mod Loader is a plugin for GTAV. With this plugin you no more need to import modified files back into the real RPF archive. You can simply put them in a mod directory and write a configuration file that describes in which RPF archive your files belong. This plugin does NOT modify RPF archives. The game will, however, think the RPF archives are modified.

1) How it works:
No game code and no files are modified by this plugin. Only a user-mode hook of the windows functions for reading files is applied to the game. When the game then reads an RPF file, this plugin returns, instead of the real RPF file, only some parts of the real RPF file combined with your modified files.

2) How to compile:
To compile the project, there are several dependencies that must be available in the library directory.

2.1) Boost:
URL: http://www.boost.org
Version: 1.59

- Download and unpack the library into lib\boost-1.59

2.2) Crypto++:
URL: http://www.cryptopp.com
Version: 5.6.2

- Download an unpack the library into lib\cryptocpp-5.6
- In crpylib project configuration: change runtime library from Multithreaded-Debug to Multithreaded-Debug-DLL in debug mode
- In crpylib project configuration: change runtime library from Multithreaded to Multithreaded-DLL in release mode mode
- Compile the library as x64 Debug and x64 Release

2.3) MHook:
URL: http://codefromthe70s.org/mhook23.aspx
Version: 2.3

- Download unpack the library into lib\mhook-2.3
- In project configuration: Change output from exe to lib
- In project configuration: Do not use precompiled header
- Compile the library as x64 Debug and x64 Release

3) How to use:
- Copy ModLoader.asi into your GTAV folder.
- Create a new folder with the name 'packages' in your GTAV folder.
- In this packages folder, create a new folder for each of your mods.
- Copy all the files of your mod into your mod folder.
- In your mod folder, also create a textfile with the name 'package.config' or copy and modify one of the examples. This is then your configuration-file.
- Write the xml-configuration. There is a complete example (with dummy files only) available.
- Run the game. The first start can take several minutes.
