cd %~dp0
dotnet run --project asset-cooker/AssetCooker.csproj
xcopy ..\..\assets\textures\hv\materials\hi-rez\*.sexml ..\..\assets-cooked\textures\hv\materials\hi-rez\*.sexml /Y
xcopy ..\..\assets\textures\hv\materials\hi-rez\default_n.dds  ..\..\assets-cooked\textures\hv\materials\hi-rez\ /Y

