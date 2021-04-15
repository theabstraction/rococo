cd /D "%~dp0"
if EXIST "Bin\x64Release\sexy.bennyhill.exe" (
echo "Building interfaces using Benny Hill..."
Bin\x64Release\sexy.bennyhill.exe SS\sexy.nativelib.coroutines\ coroutines.sxh null
) else (
"Building interfaces using Benny Hill (Debug)..."
Bin\x64Debug\sexy.bennyhill.exe SS\sexy.nativelib.coroutines\ coroutines.sxh null
)