cd %~dp0
cd ..

copy content\scripts\interop\rococo\mplat\mplat_types.sxy packages\mhost\MHost
copy content\scripts\interop\rococo\mplat\mplat_sxh.sxy packages\mhost\MHost
copy content\scripts\interop\rococo\mplat\mplat_gui_sxh.sxy packages\mhost\MHost
copy content\scripts\interop\rococo\mplat\types.sxy packages\mhost\MHost

IF EXIST "gen\bin\win64\Debug\rococo.packager.exe" (
    gen\bin\win64\Debug\rococo.packager.exe packages\mhost content\packages\mhost_1000.sxyz
) ELSE (
    gen\bin\win64\Release\rococo.packager.exe packages\mhost content\packages\mhost_1000.sxyz
)

cd %~dp0