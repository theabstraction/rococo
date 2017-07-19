#include <sexy.lib.s-parser.h>
#include <sexy.lib.util.h>
#include <sexy.lib.script.h>

#ifdef _DEBUG
# pragma comment(lib, "rococo.sexy.ide.debug.lib")
#else
# pragma comment(lib, "rococo.sexy.ide.lib")
#endif

#include "..\dx11.renderer\dx11.imports.inl"

#ifdef _DEBUG
#pragma comment(lib, "rococo.widgets.debug.lib")
#else
#pragma comment(lib, "rococo.widgets.lib")
#endif