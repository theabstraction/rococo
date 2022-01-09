cd %~dp0
mkdir content\packages
echo "Building 3rd party libraries"
call build.3rd_party.release.bat
echo "Building Sexy Scripting Language"
call sexy\build.release.bat
