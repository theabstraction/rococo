@echo off
cd %~dp0
cd ..\..\
if NOT EXIST "rococo.guid.txt" goto :noguid
if "%~1"=="" goto :badargs
if "%~2"=="" goto :badargs
if "%~3"=="" goto :badargs
if "%~4"=="" goto :badargs
if "%~5"=="" goto :badargs
echo rococo.sxh.bat - Working Directory: %cd%
echo NMAKE /NOLOGO /F "build\tools\rococo.sxh.mak" CONFIG=%1 SOURCE_ROOT=%2 INTEROP_SUBDIR=%3 SXH_FILE=%4 XC_FILE=%5
NMAKE /NOLOGO /D /F "build\tools\rococo.sxh.mak" CONFIG=%1 SOURCE_ROOT=%2 INTEROP_SUBDIR=%3 SXH_FILE=%4 XC_FILE=%5
goto :end
:badargs
@echo "Incorrect arguments. Usage: rococo.sxh.bat <1: CONFIG> <2:CPP_SOURCE-ROOT> <3:SXY_INTEROP_SUBDIR> <4:SXH_RELATIVE_TO_SOURCE_ROOT> <5:XC_FILE_RELATIVE_TO_SOURCE_ROOT>"
goto :end
:noguid
@echo "Could not find rococo.guid.txt. Check batch file and modify the cd command at the top of the file to cd into the rococo root dir
:end

