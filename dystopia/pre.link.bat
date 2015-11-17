..\x64\Release\build.maker dystopia.version
@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat" amd64
cl.exe /c /MDd dystopia.version.cpp /Fodystopia.version.Debug.obj
cl.exe /c /MD dystopia.version.cpp /Fodystopia.version.Release.obj
