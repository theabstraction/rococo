cd %~dp0..\..\
:: We are now in rococo root
gen\bin\win64\debug\rococo.sexy.cmd.exe installation=build\projects\cmd-content\ -I run=!scripts/sexyacc.sxy
cd %~dp0