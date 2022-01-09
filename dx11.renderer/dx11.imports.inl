#ifdef _DEBUG
# pragma comment(lib, "dx11.renderer.debug.lib")
# pragma comment(lib, "rococo.tiff.debug.lib")
# pragma comment(lib, "rococo.jpg.debug.lib")
# pragma comment(lib, "rococo.windows.debug.lib")
# pragma comment(lib, "rococo.fonts.debug.lib")
#else
# pragma comment(lib, "dx11.renderer.lib")
# pragma comment(lib, "rococo.tiff.lib")
# pragma comment(lib, "rococo.jpg.lib")
# pragma comment(lib, "rococo.windows.lib")
# pragma comment(lib, "rococo.fonts.lib")
#endif

#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "d3d11.lib")
