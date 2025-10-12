echo off
SET PLUGIN_BUILD_HOME=%~dp0Plugins\
set RococoHome=%~dp0..\..\
set "UE=C:\Program Files\Epic Games\UE_5.6\Engine\Build\BatchFiles\"
::set UE=..\..\..\UE\Engine\Build\BatchFiles\
echo %UE%
cd /d "%UE%"
dir -p
CALL RunUAT.bat BuildPlugin -Plugin=%~dp0Plugins\RococoBuild\RococoBuild.uplugin     -Package=%~dp0Exported  -Rocket
set  Rococo-Include=%RococoHome%source\rococo\include\
set  Sexy-Include=%RococoHome%source\rococo\sexy\Common\
echo "*************************************************"
echo %Rococo-Include%
echo "*************************************************
CALL RunUAT.bat BuildPlugin -Plugin=%~dp0Plugins\RococoOS\RococoOS.uplugin           -Package=%~dp0Exported  -Rocket
CALL RunUAT.bat BuildPlugin -Plugin=%~dp0Plugins\RococoUtil\RococoUtil.uplugin       -Package=%~dp0Exported  -Rocket
CALL RunUAT.bat BuildPlugin -Plugin=%~dp0Plugins\RococoZLib\RococoZLib.uplugin       -Package=%~dp0Exported  -Rocket
CALL RunUAT.bat BuildPlugin -Plugin=%~dp0Plugins\RococoJPEGLib\RococoJPEGLib.uplugin -Package=%~dp0Exported  -Rocket
CALL RunUAT.bat BuildPlugin -Plugin=%~dp0Plugins\RococoTiffLib\RococoTiff.uplugin    -Package=%~dp0Exported  -Rocket
CALL RunUAT.bat BuildPlugin -Plugin=%~dp0Plugins\RococoGui\RococoGui.uplugin         -Package=%~dp0Exported  -Rocket

