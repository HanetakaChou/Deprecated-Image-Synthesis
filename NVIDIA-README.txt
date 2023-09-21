NVIDIA HairWorks
================

NVIDIA HairWorks SDK, Viewer, and sample applications

Directories and files
=====================

bin : Executables and DLL files
docs : Documentations for DCC Plugin, Viewer and SDK
include : Header files for SDK
media : Sample art resources
samples : Sample projects for HairWorks SDK integration

How to run HairWorks Viewer
===========================

Run bin\win64\FurViewer.win64.exe
 (or bin\win32\FurViewer.win32.exe on 32 bit OS)

For more info, select from menu: Help > Documentation

The HairWorks Viewer needs the DirectX 2010 Runtime. If you get an error about loading a dll when trying to start the viewer download and install the following:

DirectX 2010 Runtime
https://www.microsoft.com/en-us/download/details.aspx?id=8109

How to build sample projects
============================

The HairWorks SDK ships with binary builds of the samples in the bin directory.

All samples build with the Windows Platform SDK. The Platform SDK is part of Windows 8.1 and later OSes, and is also shipped as part of Visual Studio 2012-2015. If you are running on a prior OS and compiler you may need to install the platform SDK. The steps are covered on the section on Visual Studio 2010.

The samples can be found in the samples directory. 

The samples/build directory contains Visual Studio projects to build the samples.
 
Choose the version appropriate for your Visual Studio version and whether you want a 32 or 64 bit build. 

If you have a newer version of Visual Studio, use the project version closest to your version, and upgrade as necessary.

* Visual Studio 2010 *

To build with Visual Studio 2010 takes some extra care because VS 2010 does not ship with the Windows Platform SDK. 

The Windows 8.1 Platform SDK can be found here..

https://msdn.microsoft.com/en-us/windows/desktop/bg162891.aspx

The sample build projects assumes you have the platform SDK installed to the usual place which is typically

c:\Program Files(x86)\Windows Kits\8.1\

If you do not have the SDK located here, or wish to use a different Platform SDK then you will have to change:

1) The path to the shader compiler (right click on the shader source, and change Properties/Custom Build Tool set the path)
2) The include directories (In Configuration Properties->C/C++/General/Additional Include Directories)
Change these to the appropriate locataction in the platform sdk
$(ProgramFiles)/Windows Kits/8.1/Include/um
$(ProgramFiles)/Windows Kits/8.1/Include/shared
3) The lib paths (In Configuration Properties->Linker->Additional Library Directories)
Change this to the appropriate location of the platform SDK
For x64/Win64 
$(ProgramFiles)/Windows Kits/8.1/lib/winv6.3/um/x64
For x86/Win32 
$(ProgramFiles)/Windows Kits/8.1/lib/winv6.3/um/x86

When you get the executable to build, one final thing to be aware of is that the executable may not know where to find the shader compiler dll. You can copy from the platform SDK into executable directory for the samples. Typically copy...

d3dcompiler_47.dll

For x64/win64
C:\Program Files (x86)\Windows Kits\8.1\bin\x64 -> \bin\win64

For x86/Win32
C:\Program Files (x86)\Windows Kits\8.1\bin\x86 -> \bin\win32

Documentation
=============

See docs/guide/index.html and docs/api/index.html

