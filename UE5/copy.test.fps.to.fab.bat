@echo off
set source=%~dp0RococoTestFPS\
set target=%~dp0..\..\Rococo.Plugins\RococoGuiUltra.Test\
set ultra=RococoTestFPS
echo source=%source%
echo target=%target%
:: Rename the plugin here so the UE5 browser can distinguish between the project built with plugins, and the project requiring the merged engine plugin (ultra)
@echo on
xcopy %Source%*.uproject %Target% /R /Y
xcopy %Source%Source\RococoTestFPS\ %Target%Source\%ultra%\ /R /Y /E
xcopy %Source%Content\ %Target%Content\ /R /Y /E
xcopy %Source%Config\ %Target%Config\ /R /Y /E
