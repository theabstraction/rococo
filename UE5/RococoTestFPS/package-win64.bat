@echo off
cd %~dp0
set UE=..\..\..\UE\Engine\Build\BatchFiles\
%UE%RunUAT.bat BuildCookRun -project=%~dp0RococoTestFPS.uproject -platform=Win64 -clientconfig=Shipping -cook -stage -package -stagingdirectory=%~dp0Packaged > %~dp0>package-win64.log 2>&1
