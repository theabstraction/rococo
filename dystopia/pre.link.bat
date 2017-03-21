cd %1
.\bin\build.maker.exe %2dystopia.version
@echo off
call "C:\VS2015\VC\vcvarsall.bat" amd64
cl.exe %3 /c %2dystopia.version.cpp /Fo%1temp/dystopia/version.obj

