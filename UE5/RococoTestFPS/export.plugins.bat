@echo off
cd %~dp0
set UE="C:\Program Files\Epic Games\UE_5.6\"
%UE%Engine\Build\BatchFiles\RunUAT.bat BuildPlugin -Plugin=Plugins\RococoBuild\RococoBuild.uplugin     -Package=Exported\RococoBuild    -Rocket
%UE%Engine\Build\BatchFiles\RunUAT.bat BuildPlugin -Plugin=Plugins\RococoOS\RococoOS.uplugin           -Package=Exported\RococoOS       -Rocket
%UE%Engine\Build\BatchFiles\RunUAT.bat BuildPlugin -Plugin=Plugins\RococoGui\RococoGui.uplugin         -Package=Exported\RococoGui      -Rocket
%UE%Engine\Build\BatchFiles\RunUAT.bat BuildPlugin -Plugin=Plugins\RococoJPEGLib\RococoJPEGLib.uplugin -Package=Exported\RococoJPEGLib  -Rocket
%UE%Engine\Build\BatchFiles\RunUAT.bat BuildPlugin -Plugin=Plugins\RococoTiffLib\RococoTiff.uplugin    -Package=Exported\RococoTiff     -Rocket
%UE%Engine\Build\BatchFiles\RunUAT.bat BuildPlugin -Plugin=Plugins\RococoUtil\RococoUtil.uplugin       -Package=Exported\RococoUtil     -Rocket
%UE%Engine\Build\BatchFiles\RunUAT.bat BuildPlugin -Plugin=Plugins\RococoZLib\RococoZLib.uplugin       -Package=Exported\RococoZLib     -Rocket
