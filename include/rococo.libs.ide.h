#pragma once

#ifdef _WIN32
# ifdef _DEBUG
#  pragma comment(lib, "rococo.sexy.ide.Debug.lib")
#  pragma comment(lib, "rococo.windows.Debug.lib")
# else
#  pragma comment(lib, "rococo.sexy.ide.lib")
#  pragma comment(lib, "rococo.windows.lib")
# endif
#endif