@echo off

if [%1]==[] goto usage
goto makestuff
:usage
@echo Usage: %0 <debug|release>
exit /B 1

:makestuff
cd %~dp0
IF NOT EXIST ..\..\SexyStudioBin\NUL mkdir ..\..\SexyStudioBin\
nmake -nologo -f ship.sexystudio.mak all CONFIG=%1
