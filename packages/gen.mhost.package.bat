cd %~dp0
cd ..
echo "Generating mhost package"

IF EXIST "..\bin\rococo.packager.exe" (
    bin\rococo.packager.exe packages\mhost content\packages\mhost_1000.sxyz
) ELSE (
    bin\rococo.packager.debug.exe packages\mhost content\packages\mhost_1000.sxyz
)

