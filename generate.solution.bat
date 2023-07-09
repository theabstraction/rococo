cd %~dp0
set sharpmake_exe=..\sharpmake\SharpMake.Application\bin\Debug\net6.0\Sharpmake.Application.exe

%sharpmake_exe% "/sources(@"sharpmake\rococo.sharpmake.cs") /verbose"