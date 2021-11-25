#ifdef _WIN32
# ifdef _DEBUG
#  pragma comment(lib, "sexy.util.Debug.lib")
#  pragma comment(lib, "rococo.util.debug.lib")
#  pragma comment(lib, "rococo.misc.utils.debug.lib")
# else
#  pragma comment(lib, "sexy.util.Release.lib")
#  pragma comment(lib, "rococo.utils.lib")
#  pragma comment(lib, "rococo.misc.util.lib")
# endif
#endif