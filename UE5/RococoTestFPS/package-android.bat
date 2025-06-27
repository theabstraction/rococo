@echo off
cd %~dp0
SET UE=%~dp0..\..\..\UE\
set UEBF= %UE%Engine\Build\BatchFiles\
set devContent=..\..\content\
mkdir Packaged
set DROID=Packaged\Android_ASTC\
mkdir %DROID%
mkdir %DROID%RococoTestFPS
mkdir %DROID%RococoTestFPS\Content
set packagedRococoContent=%DROID%RococoTestFPS\Content\rococo.content\
mkdir %packagedRococoContent%
xcopy %devContent%tests\*.sexml %packagedRococoContent%\tests\ /Y /D
xcopy %devContent%tests\*.sexml %packagedRococoContent%\tests\ /Y /D
xcopy %devContent%textures\test\*.jpg %packagedRococoContent%\textures\test\ /Y /D
xcopy %devContent%textures\test\*.jpg %packagedRococoContent%\textures\test\ /Y /D
xcopy %devContent%textures\toolbars\MAT %packagedRococoContent%\textures\toolbars\MAT\ /Y /D
xcopy %devContent%textures\prompts %packagedRococoContent%\textures\prompts\ /Y /D /S
xcopy %devContent%textures\toolbars\3rd-party\www.aha-soft.com\ %packagedRococoContent%\textures\toolbars\3rd-party\www.aha-soft.com\ /F /Y /D
set logfile=%~dp0package-android.log

echo BuildCookRun underway. Please be patient

%UEBF%RunUAT.bat ^
-ScriptsForProject=%~dp0RococoTestFPS.uproject^
Turnkey -command=VerifySdk -platform=Android -UpdateIfNeeded -EditorIO -EditorIOPort=51082^
-project=%~dp0RococoTestFPS.uproject^
BuildCookRun -nop4 -utf8output -nocompileeditor -skipbuildeditor -cook^
-project=%~dp0RococoTestFPS.uproject -target=RococoTestFPS^ 
-unrealexe="D:\work\rococo\UE5\RococoTestFPS\Binaries\Win64\RococoTestFPSEditor-Cmd.exe" -plat^
form=Android  -cookflavor=ASTC -stage -archive -package -build -pak -iostore -compressed -prereqs^
-archivedirectory=%~dp0RococoTestFPS/Packaged -clientconfig=Development" -nocompile -nocompileuat^
REM > %logfile% 2>&1
