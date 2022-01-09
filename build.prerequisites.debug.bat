cd %~dp0
echo "Building 3rd party libraries (DEBUG)"
call build.3rd_party.debug.bat
echo "Building Sexy Scripting Language (DEBUG)"
call sexy\build.debug.bat
