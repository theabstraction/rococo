cd %~dp0
cd ..
echo "Generating tables package"
bin\rococo.packager.exe packages\tables content\packages\tables_1000.sxyz
