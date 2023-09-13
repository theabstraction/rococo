cd /D "%~dp0"
if EXIST "..\in\x64Release\sexy.bennyhill.exe" (
echo "Building interfaces using Benny Hill..."
bin\sexy.bennyhill.exe SS\sexy.nativelib.coroutines\ coroutines.sxh null
) else (
echo "Building interfaces using Benny Hill (Debug)..."
bin\sexy.bennyhill.debug.exe SS\sexy.nativelib.coroutines\ coroutines.sxh null
)