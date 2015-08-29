To compile the project, there are several dependencies that must be available in the library directory.

Boost
=====
URL: http://www.boost.org
Version: 1.59

- Download and unpack the library into lib\boost-1.59


Crypto++
========
URL: http://www.cryptopp.com
Version: 5.6.2

- Download an unpack the library into lib\cryptocpp-5.6
- In crpylib project configuration: change runtime library from Multithreaded-Debug to Multithreaded-Debug-DLL in debug mode
- In crpylib project configuration: change runtime library from Multithreaded to Multithreaded-DLL in release mode mode
- Compile the library as x64 Debug and x64 Release


MHook
=====
URL: http://codefromthe70s.org/mhook23.aspx
Version: 2.3

- Download unpack the library into lib\mhook-2.3
- In project configuration: Change output from exe to lib
- In project configuration: Do not use precompiled header
- Compile the library as x64 Debug and x64 Release
