@echo off
cd %~dp0
if "%~1"=="" goto :no_config
nmake -nologo -f package.mak all CONFIG=%1
goto :end
:no_config
echo "usage: package.bat <CONFIGURATION> where <CONFIGURATION> is usually one of {debug|release}"

:end
