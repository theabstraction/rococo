@echo off
cd %~dp0
nmake -nologo -f build.shaders.mak all CONFIG=%1
goto :end
:end