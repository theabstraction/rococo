#ifndef ROCOCO_LIBS_H
# define ROCOCO_LIBS_H
# ifdef _WIN32

# ifdef _DEBUG
#  pragma comment(lib, "rococo.windows.debug.lib")
#  pragma comment(lib, "rococo.util.debug.lib")
#  pragma comment(lib, "rococo.tiff.debug.lib")
#  pragma comment(lib, "rococo.jpg.debug.lib")
#  pragma comment(lib, "rococo.zlib.debug.lib")
#  pragma comment(lib, "rococo.maths.debug.lib")
#  pragma comment(lib, "rococo.sexy.ide.debug.lib")
# else
#  pragma comment(lib, "rococo.windows.lib")
#  pragma comment(lib, "rococo.util.lib")
#  pragma comment(lib, "rococo.tiff.lib")
#  pragma comment(lib, "rococo.jpg.lib")
#  pragma comment(lib, "rococo.zlib.lib")
#  pragma comment(lib, "rococo.maths.lib")
#  pragma comment(lib, "rococo.sexy.ide.lib")
# endif

# endif
#endif