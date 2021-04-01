cd \work\rococo\sexy\
del Bin\x64Debug\*.exe
del Bin\x64Debug\*.dll
del ..\lib\sexy*.debug.lib
msbuild sexy.sln -p:Configuration=Debug -t:Build -p:Platform=x64 -m