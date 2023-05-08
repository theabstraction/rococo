#pragma once

#ifdef _WIN32
# ifdef _DEBUG
#  pragma comment(lib, "rococo.windows.debug.lib")
#  pragma comment(lib, "rococo.util.debug.lib")
#  pragma comment(lib, "rococo.tiff.debug.lib")
#  pragma comment(lib, "rococo.jpg.debug.lib")
#  pragma comment(lib, "rococo.zlib.debug.lib")
#  pragma comment(lib, "rococo.maths.debug.lib")
#  pragma comment(lib, "rococo.sexy.ide.debug.lib")
#  pragma comment(lib, "rococo.misc.utils.debug.lib")
#  pragma comment(lib, "sexy.script.Debug.lib")
#  pragma comment(lib, "rococo.util.ex.debug.lib")
# else
#  pragma comment(lib, "rococo.windows.lib")
#  pragma comment(lib, "rococo.util.lib")
#  pragma comment(lib, "rococo.tiff.lib")
#  pragma comment(lib, "rococo.jpg.lib")
#  pragma comment(lib, "rococo.zlib.lib")
#  pragma comment(lib, "rococo.maths.lib")
#  pragma comment(lib, "rococo.sexy.ide.lib")
#  pragma comment(lib, "rococo.misc.utils.lib")
#  pragma comment(lib, "sexy.script.Release.lib")
#  pragma comment(lib, "rococo.util.ex.lib")
# endif
#endif