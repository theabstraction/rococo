cd %~dp0
copy sexy\NativeSource\*.sxy content\scripts\native\
copy content\scripts\mplat* packages\mhost\MHost\
copy content\scripts\types.sxy packages\mhost\MHost\types.sxy
call packages\gen.mhost.package.bat
echo "Completed"