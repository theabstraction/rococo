cd %~dp0
cd ..
echo "Generating mhost package"

copy content\scripts\mplat_sxh.sxy packages\mhost\MHost
copy content\scripts\types.sxy packages\mhost\MHost

IF EXIST "bin\rococo.packager.exe" (
    bin\rococo.packager.exe packages\mhost content\packages\mhost_1000.sxyz
) ELSE (
    bin\rococo.packager.debug.exe packages\mhost content\packages\mhost_1000.sxyz
)

