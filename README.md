# rococo and sexy librarys - mark.anthony.taylor@gmail.com.

Build instructions - the current build process is optimized for how I work day-to-day.

Install the repo at C:\work\rococo\...  I use absolute paths rather than relative, it makes writing scripts easier

Batch build..clean everything debug and release

First build the rococo.util library ONLY in debug AND release mode.

Run this batch file once: C:\work\rococo\sexy\SS\sexy.nativelib.coroutines\build.bat
Descend into the sexy folder, open the sexy.sln do a batch rebuild on debug and release mode
You can run the sexy.script.test in release, it should complete in under 3 seconds on any modern PC

Return to the rococo libs and do a rebuild.

There are makefiles for OSX, but I only ever fix OSX when I notice it is broken, and its an unofficial part
of the distrbution - but it is in use commercially, so when I fix it, it does function.

Since MPLAT (Media-PLATform) uses DX11 as renderer and XAudio2 as sound, and I haven't implemented an OpenGL renderer or OpenAL sound engine
then the MPlatform currently needs work to get it compiled for OSX. The Win32 specific stuff is fairly well quaranteened though.

There is also no IDE script debugger for OSX, so plan to debug your scripts on a PC then port to OSX.

Both Hyperverse and MHOST depend on MPLAT

If you have any problems drop me an email
