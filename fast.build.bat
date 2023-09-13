@echo off
cd %~dp0
if "%~1"=="" goto :no_config
if "%~2"=="" goto :no_directive
if "%~3"=="" goto :no_verbosity
cd build
msbuild rococo.fastdev.sln %3 -m -p:Configuration="%1" -t:%2 -p:Platform="x64" -maxCpuCount:6
goto :end
:no_config
@echo "missing CONFIG (parameter 1) usage: package.bat <CONFIGURATION> <DIRECTIVE> <OPTIONAL_MSBUILD_SWITCH>,  where <CONFIGURATION> is usually one of {debug|release} and <DIRECTIVE is usually one of {build|rebuild|clean}"
goto :end
:no_verbosity
@echo missing VERBOSITY (parameter 3)
goto end:
:no_directive
@echo "missing DIRECTIVE (parameter 2) usage: package.bat <CONFIGURATION> <DIRECTIVE> <OPTIONAL_MSBUILD_SWITCH>,  where <CONFIGURATION> is usually one of {debug|release} and <DIRECTIVE is usually one of {build|rebuild|clean}"
:end
