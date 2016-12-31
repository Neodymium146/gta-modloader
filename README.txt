Mod Loader is a plugin for GTAV. With this plugin you no more need to import modified files back into the real RPF archive. You can simply put them in a mod directory and write a configuration file that describes in which RPF archive your files belong. This plugin does NOT modify RPF archives. The game will, however, think the RPF archives are modified.

1) How it works:
No game code and no files are modified by this plugin. Only a user-mode hook of the windows functions for reading files is applied to the game. When the game then reads an RPF file, this plugin returns, instead of the real RPF file, only some parts of the real RPF file combined with your modified files.

2) How to compile:
To compile the project, there are several dependencies that must be available in the library directory.

2.1) Boost:
URL: http://www.boost.org
Version: 1.63

- Download and unpack the library into lib\boost-1.63

2.2) Crypto++:
URL: http://www.cryptopp.com
Version: 5.6.5

- Download an unpack the library into lib\cryptocpp-5.6.5
- In crpylib project debug configuration: change runtime library from Multithreaded-Debug to Multithreaded-Debug-DLL
- In crpylib project release configuration: change runtime library from Multithreaded to Multithreaded-DLL
- Compile the library as Debug-x64 and Release-x64

2.3) MHook:
URL: http://codefromthe70s.org/mhook24.aspx
Version: 2.4

- Download unpack the library into lib\mhook-2.4
- In project all configurations: Change output from exe to lib
- In project all configurations: Do not use precompiled header
- Compile the library as Debug-x64 and Release-x64

3) How to use:
- Copy ModLoader.asi into your GTAV folder.
- Create a new folder with the name 'packages' in your GTAV folder.
- In this packages folder, create a new folder for each of your mods.
- Copy all the files of your mod into your mod folder.
- In your mod folder, also create a textfile with the name 'package.config' or copy and modify one of the examples. This is then your configuration-file.
- Write the xml-configuration. There is a complete example (with dummy files only) available.
- Run the game. The first start can take several minutes.
