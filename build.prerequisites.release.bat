mkdir \work\rococo\content\packages
echo "Building 3rd party libraries"
call \work\rococo\build.3rd_party.release.bat
echo "Building Sexy Scripting Language"
call \work\rococo\sexy\build.release.bat
