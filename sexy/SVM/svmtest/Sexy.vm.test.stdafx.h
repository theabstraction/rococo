// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once



#include "sexy.types.h"
#include "sexy.strings.h"

#ifdef _WIN32
# define WIN32_LEAN_AND_MEAN
# include <rococo.win32.target.win7.h>
# include <Windows.h>
# include <tchar.h>
# include <intrin.h>
#else
# include <unistd.h>
# include <sys/sysctl.h>
# include <mach/mach.h>
# include <mach/mach_time.h>
#endif

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>




// TODO: reference additional headers your program requires here
