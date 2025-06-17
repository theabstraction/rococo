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

#include "sexy.vm.stdafx.h"
#include "sexy.vm.os.h"

#include "../../Common/Sexy.S-Parser.h"

#include <rococo.strings.h>

#include <rococo.win32.target.win7.h>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <excpt.h>
#include <intrin.h>

#include <rococo.api.h>
#include <rococo.os.h>
#include <rococo.sexy.allocators.h>

using namespace Rococo;
using namespace Rococo::VM;

namespace
{
	EXCEPTIONCODE SysToSVM(uint32 code)
	{
		switch(code)
		{
		case EXCEPTION_ACCESS_VIOLATION:
			return EXCEPTIONCODE_BAD_ADDRESS;
		case EXCEPTION_DATATYPE_MISALIGNMENT:
			return EXCEPTIONCODE_BAD_ALIGN;
		default:
			return EXCEPTIONCODE_UNKNOWN;
		}
	}
}

namespace Rococo { namespace VM { namespace OS 
{
	EXECUTERESULT ExecuteProtectedLayer0(IVirtualMachine& vm, FN_CODE fnCode, void* context, EXCEPTIONCODE& exceptionCode, bool arg)
	{
		exceptionCode = EXCEPTIONCODE_NONE;

		try
		{
			return fnCode(context, arg);
		}
		catch (Rococo::Sex::ParseException& e)
		{
			auto start = e.Start();
			auto end = e.End();

			cstr sourceFile;
			auto* s = e.Source();
			sourceFile = (s != nullptr) ? s->Tree().Source().Name() : "<unknown>";
	
			char fullMessage[1024];
			Strings::SafeFormat(fullMessage, 1024, "(%d.%d) to (%d.%d). Source: %s.\n%s", start.x, start.y, end.x, end.y, sourceFile, e.Message());
			vm.Core().Log(fullMessage);
			return EXECUTERESULT_THROWN;
		}
		catch (IException& ex)
		{
			char msg[4096];
			Rococo::OS::BuildExceptionString(msg, sizeof msg, ex, true);
			vm.Core().Log(msg);
			return EXECUTERESULT_THROWN;
		}
	}

	EXECUTERESULT ExecuteProtected(IVirtualMachine& vm, FN_CODE fnCode, void* context, EXCEPTIONCODE& exceptionCode, bool arg)
	{
		__try
		{
			return ExecuteProtectedLayer0(vm, fnCode, context, exceptionCode, arg);
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			OUT exceptionCode = SysToSVM(GetExceptionCode());
			return EXECUTERESULT_SEH;
		}
	}

	EXECUTERESULT ExecuteProtectedLayer0(IVirtualMachine& vm, FN_CODE1 fnCode, void* context, EXCEPTIONCODE& exceptionCode)
	{
		exceptionCode = EXCEPTIONCODE_NONE;

		try
		{
			return fnCode(context);
		}
		catch (IException& ex)
		{
			vm.Core().Log(ex.Message());
			return EXECUTERESULT_THROWN;
		}
	}

	EXECUTERESULT ExecuteProtected(IVirtualMachine& vm, FN_CODE1 fnCode, void* context, EXCEPTIONCODE& exceptionCode)
	{
		exceptionCode = EXCEPTIONCODE_NONE;

		__try
		{
			return ExecuteProtectedLayer0(vm, fnCode, context, exceptionCode);
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			OUT exceptionCode = SysToSVM(GetExceptionCode());
			return EXECUTERESULT_SEH;
		}
	}

	int64 TimerTicks()
	{
		LARGE_INTEGER ticks;
		QueryPerformanceCounter(&ticks);
		return ticks.QuadPart;
	}

	int64 TimerHz()
	{
		LARGE_INTEGER hz;
		QueryPerformanceFrequency(&hz);
		return hz.QuadPart;
	}

	void* AllocAlignedMemory(size_t nBytes, int32 alignment)
	{
		return Rococo::Memory::AlignedAlloc(nBytes, alignment, Rococo::Memory::AllocateSexyMemory);
	}

	void FreeAlignedMemory(void* data)
	{
		return Rococo::Memory::AlignedFree(data, Rococo::Memory::FreeSexyUnknownMemory);
	}
}}} // Rococo::VM::OS