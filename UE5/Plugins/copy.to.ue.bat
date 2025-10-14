echo off
set EnginePlugins="C:\Program Files\Epic Games\UE_5.6\Engine\Plugins"
set PluginName=RococoGuiUltra
set Source=%~dp0..\..\..\Rococo.Plugins\%PluginName%
set Target=%EnginePlugins%\%PluginName%\
echo Source: %Source%
echo Target: %Target%
xcopy %Source% %Target% /R /Y /E
