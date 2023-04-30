// stdafx.cpp : source file that includes just the standard includes
// sexydotnethost.pch will be the pre-compiled header
// stdafx.obj will contain the pre-compiled type information

#include "stdafx.h"
#include "sexy.lib.script.h"

#ifdef _DEBUG
#  pragma comment(lib, "rococo.util.debug.lib")
#  pragma comment(lib, "sexy.s-parser.Debug.lib")
# else
#  pragma comment(lib, "rococo.util.lib")
#  pragma comment(lib, "sexy.s-parser.Release.lib")
#endif