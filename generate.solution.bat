cls
cd %~dp0
set sharpmake_exe=..\sharpmake\SharpMake.Application\bin\Debug\net6.0\Sharpmake.Application.exe
set target_file=%~dp0build\sharpmake\rococo.sharpmake.cs
%sharpmake_exe% /sources(@'%target_file%') /verbose