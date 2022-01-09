cd %~dp0
msbuild rococo.3rd-party.sln -p:Configuration=Debug -t:Build -p:Platform=x64
msbuild rococo.3rd-party.sln -p:Configuration=Release -t:Build -p:Platform=x64