cd /d %~dp0
dotnet run --project asset-cooker/AssetCooker.csproj
xcopy ..\..\assets\textures\hv\materials\hi-rez\*.sexml ..\..\assets-cooked\textures\hv\materials\hi-rez\*.sexml /Y /I
xcopy ..\..\assets\textures\hv\materials\hi-rez\default_n.dds  ..\..\assets-cooked\textures\hv\materials\hi-rez\ /Y /I

