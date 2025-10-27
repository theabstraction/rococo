@echo off
set source=%~dp0
set target=%~dp0..\..\..\..\Rococo.Plugins.Package\RococoGuiUltra.Extras\Docs\1.2\
echo source=%source%
echo target=%target%
@echo on
xcopy %Source%*.html %Target% /R /Y
xcopy %Source%Images\*.* %Target%Images\ /Y
