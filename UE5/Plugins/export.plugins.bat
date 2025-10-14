@echo off
SET PLUGIN_BUILD_HOME=%~dp0Plugins\
set RococoHome=%~dp0..\..\
set ExportDir=%RococoHome%..\Rococo.Plugins\
set "UE=C:\Program Files\Epic Games\UE_5.6\Engine\Build\BatchFiles\"
::set UE=..\..\..\UE\Engine\Build\BatchFiles\
cd /d "%UE%"
CALL RunUAT.bat BuildPlugin -Plugin=%~dp0RococoGuiUltra\RococoGuiUltra.uplugin -Package=%ExportDir%RococoGuiUltra -Rocket
cd /d %~dp0
