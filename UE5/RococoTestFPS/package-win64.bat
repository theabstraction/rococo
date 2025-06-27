@echo off
cd %~dp0
set UE=..\..\..\UE\Engine\Build\BatchFiles\
set devContent=..\..\content\
mkdir Packaged
mkdir Packaged\Windows
mkdir Packaged\Windows\RococoTestFPS
mkdir Packaged\Windows\RococoTestFPS\Content
set packagedRococoContent=Packaged\Windows\RococoTestFPS\Content\rococo.content\
mkdir %packagedRococoContent%
xcopy %devContent%tests\*.sexml %packagedRococoContent%\tests\ /Y /D
xcopy %devContent%tests\*.sexml %packagedRococoContent%\tests\ /Y /D
xcopy %devContent%textures\test\*.jpg %packagedRococoContent%\textures\test\ /Y /D
xcopy %devContent%textures\test\*.jpg %packagedRococoContent%\textures\test\ /Y /D
xcopy %devContent%textures\toolbars\MAT %packagedRococoContent%\textures\toolbars\MAT\ /Y /D
xcopy %devContent%textures\prompts %packagedRococoContent%\textures\prompts\ /Y /D /S
xcopy %devContent%textures\toolbars\3rd-party\www.aha-soft.com\ %packagedRococoContent%\textures\toolbars\3rd-party\www.aha-soft.com\ /F /Y /D
set logfile=%~dp0package-win64.log
echo BuildCookRun underway. Please be patient
%UE%RunUAT.bat BuildCookRun -project=%~dp0RococoTestFPS.uproject -platform=Win64 -clientconfig=Shipping -cook -stage -package -stagingdirectory=%~dp0Packaged > %logfile% 2>&1
type %logfile%
