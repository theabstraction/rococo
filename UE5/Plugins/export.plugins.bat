@echo off
SET PLUGIN_BUILD_HOME=%~dp0Plugins\
set RococoHome=%~dp0..\..\
set "UE=C:\Program Files\Epic Games\UE_5.6\Engine\Build\BatchFiles\"
::set UE=..\..\..\UE\Engine\Build\BatchFiles\
cd /d "%UE%"
CALL RunUAT.bat BuildPlugin -Plugin=%~dp0RococoGuiUltra\RococoGuiUltra.uplugin -Package=%~dp0Exported -Rocket
cd /d %~dp0
