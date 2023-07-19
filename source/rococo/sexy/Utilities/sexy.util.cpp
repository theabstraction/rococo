/*
	Sexy Scripting Language - Copright(c)2013. Mark Anthony Taylor. All rights reserved.

	https://github.com/theabstraction/rococo

	Email: mark.anthony.taylor@gmail.com

	The Sexy copyright holder is Mark Anthony Taylor, the English author of 'Lazy Bloke Ghosts of Parliament', 'Lazy Bloke in the 35th Century', 'King of the Republic', 'Origin of the Species' 
	

	1. This software is open-source. It can be freely copied, distributed, compiled and the compilation executed.
	
	1.a Modification of the software is not allowed where such modifcations fail to include this exact comment header in each source file, or alter, reduce or extend the Sexy language.
	The purpose of this limitation is to prevent different incompatible versions of the Sexy language emerging. 

	1.b You are allowed to fix bugs and implement performance improvements providing you inform Sexy's copyright owner via email. Such changes may then be incorporated in
	later versions of Sexy without compromising Sexy's copyright license.
	
	2. You are not permitted to copyright derivative versions of the source code. You are free to compile the code into binary libraries and include the binaries in a commercial application. 

	3. THERE IS NO WARRANTY FOR THE PROGRAM, TO THE EXTENT PERMITTED BY APPLICABLE LAW. EXCEPT WHEN OTHERWISE STATED IN WRITING THE COPYRIGHT HOLDERS AND/OR OTHER PARTIES PROVIDE THE PROGRAM “AS IS” WITHOUT
	WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. THE ENTIRE RISK AS TO THE QUALITY
	AND PERFORMANCE OF THE PROGRAM IS WITH YOU. SHOULD THE PROGRAM PROVE DEFECTIVE, YOU ASSUME THE COST OF ALL NECESSARY SERVICING, REPAIR OR CORRECTION.

	4. IN NO EVENT UNLESS REQUIRED BY APPLICABLE LAW OR AGREED TO IN WRITING WILL ANY COPYRIGHT HOLDER, OR ANY OTHER PARTY WHO MODIFIES AND/OR CONVEYS THE PROGRAM AS PERMITTED ABOVE, BE LIABLE TO YOU FOR
	DAMAGES, INCLUDING ANY GENERAL, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES ARISING OUT OF THE USE OR INABILITY TO USE THE PROGRAM (INCLUDING BUT NOT LIMITED TO LOSS OF DATA OR DATA BEING RENDERED
	INACCURATE OR LOSSES SUSTAINED BY YOU OR THIRD PARTIES OR A FAILURE OF THE PROGRAM TO OPERATE WITH ANY OTHER PROGRAMS), EVEN IF SUCH HOLDER OR OTHER PARTY HAS BEEN ADVISED OF THE POSSIBILITY OF
	SUCH DAMAGES.

	5. Any program that distributes this software in source or binary form must include an obvious credit to the the language, its copyright holder, and the language homepage. This must be the case in its
	principal credit screen and its principal readme file.
*/

#include "sexy.types.h"
#define ROCOCO_USE_SAFE_V_FORMAT
#include "sexy.strings.h"
#include "sexy.compiler.public.h"

#include "..\STC\stccore\sexy.validators.h"

#include "..\STC\stccore\Sexy.Compiler.h"

#include <float.h>
#include <stdarg.h>

#include "sexy.stdstrings.h"

#include <unordered_map>
#include <algorithm>

#include <rococo.api.h>

#include <rococo.allocators.inl>

using namespace Rococo::Memory;

DeclareAllocator(DefaultAllocator, SexyUtils, g_allocator)
//Rococo::Memory::AllocatorMonitor<SexyUtils> monitor;

OVERRIDE_MODULE_ALLOCATORS_WITH_FUNCTOR(g_allocator)

namespace Rococo
{
	SEXYUTIL_API void LogError(ILog& log, cstr format, ...)
	{
		va_list args;
		va_start(args, format);

		char msg[1024];
		SafeVFormat(msg, sizeof(msg), format, args);
		log.Write(msg);
	}
}

namespace Rococo
{
	namespace Script
	{
		ROCOCO_API_EXPORT void ThrowBadNativeArg(int index, cstr source, cstr message)
		{
			WriteToStandardOutput(("Error %d in %s: %s\r\n"), index, source, message);
			Throw(0, "Bad native argument: %s - %s", source, message);
		}
	}
}

#include <windows.h>
#include <debugapi.h>
#include <dbghelp.h>

#include <rococo.debugging.h>

namespace Rococo::Memory
{
	struct SexyDefaultAllocator: IAllocator
	{
		std::vector<FN_AllocatorReleaseFunction> atReleaseQueue;

		~SexyDefaultAllocator()
		{
			for (auto fn : atReleaseQueue)
			{
				fn();
			}
		}

		[[nodiscard]] void* Allocate(size_t capacity) override
		{
			return malloc(capacity);
		}

		void FreeData(void* data) override
		{
			free(data);
		}

		void* Reallocate(void* ptr, size_t capacity) override
		{
			return realloc(ptr, capacity);
		}

		void AtRelease(FN_AllocatorReleaseFunction exitFunction)
		{
			auto i = std::find(atReleaseQueue.begin(), atReleaseQueue.end(), exitFunction);
			if (i != atReleaseQueue.end())
			{
				atReleaseQueue.push_back(exitFunction);
			}
		}

		size_t EvaluateHeapSize() override
		{
			return 0;
		}
	} defaultAllocator;

	static IAllocator* globalSexyAllocator = &defaultAllocator;

	void SetSexyAllocator(IAllocator* allocator)
	{
		globalSexyAllocator = allocator == nullptr ? &defaultAllocator : allocator;
	}

#ifdef USE_STD_ALLOCATOR_FOR_SEXY
	// These are inlined in the headers, not defined here
#else
	SEXYUTIL_API void* AllocateSexyMemory(size_t nBytes)
	{
		void *buffer = globalSexyAllocator->Allocate(nBytes);
		return buffer;
	}

	SEXYUTIL_API void FreeSexyMemory(void* buffer, size_t /* nBytes */)
	{
		globalSexyAllocator->FreeData(buffer);
	}

	SEXYUTIL_API void FreeSexyUnknownMemory(void* buffer)
	{
		globalSexyAllocator->FreeData(buffer);
	}
#endif

	SEXYUTIL_API IAllocator& GetSexyAllocator()
	{
		return *globalSexyAllocator;
	}
}
