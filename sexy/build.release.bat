cd \work\rococo\sexy\
del Bin\x64Release\*.exe
del Bin\x64Release\*.dll
del ..\lib\sexy*.Release.lib
msbuild sexy.sln -p:Configuration=Release -t:Build -p:Platform=x64 -m