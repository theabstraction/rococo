cd c:\work\rococo
copy sexy\NativeSource\*.pdb bin
copy sexy\NativeSource\*.dll bin
copy sexy\NativeSource\*.sxy content\scripts\native\
call packages\gen.mhost.package.bat
echo "Completed"