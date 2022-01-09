cd %~dp0
msbuild rococo.3rd-party.sln -p:Configuration=Release -t:Build -p:Platform=x64 -m
