#include <sexy.lib.s-parser.h>
#include <sexy.lib.util.h>
#include <sexy.lib.script.h>

#include "..\rococo.os.win32\rococo.imports.inl"
#include "..\dx11.renderer\dx11.imports.inl"

#ifdef _DEBUG
#pragma comment(lib, "rococo.widgets.debug.lib")
#else
#pragma comment(lib, "rococo.widgets.lib")
#endif