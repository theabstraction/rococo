cd %~dp0
msbuild build\rococo.all.sln -m -p:Configuration="x64-Debug" -t:Rebuild -p:Platform="All Platforms"
