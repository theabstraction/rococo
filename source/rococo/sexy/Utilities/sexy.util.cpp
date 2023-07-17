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

// used in the sexy.script.test suite. Uncomment RECORD_ALLOCATION_HISTORY to activate allocation analysis.
// You can use MARK_MEMORY(msg) to output the outstanding allocation state at a particular point in the program.
// RecordAllocations(size_t nBytes) is used to report the call stack of any item allocated with sizeof item = nBytes 
//#define RECORD_ALLOCATION_HISTORY
#ifdef RECORD_ALLOCATION_HISTORY

#include <windows.h>
#include <debugapi.h>
#include <dbghelp.h>

#include <rococo.debugging.h>

namespace
{
	using namespace Rococo::Debugging;

	void GetFunctionName(HANDLE hProcess, StackFrame& sf, DWORD64 address)
	{
		char symbolBuffer[sizeof(IMAGEHLP_SYMBOL) + 255];
		PIMAGEHLP_SYMBOL symbol = (PIMAGEHLP_SYMBOL)symbolBuffer;
		symbol->SizeOfStruct = (sizeof IMAGEHLP_SYMBOL) + 255;
		symbol->MaxNameLength = 254;

		sf.moduleName[0] = 0;
		sf.functionName[0] = 0;
		sf.sourceFile[0] = 0;
		sf.lineNumber = 0;

		DWORD64 moduleBase = SymGetModuleBase64(hProcess, address);

		if (moduleBase)
		{
			GetModuleFileNameA((HINSTANCE)moduleBase, sf.moduleName, sizeof(sf.moduleName));
		}

		if (SymGetSymFromAddr(hProcess, address, NULL, symbol))
		{
			strncpy_s(sf.functionName, symbol->Name, _TRUNCATE);
		}

		DWORD  offset = 0;
		IMAGEHLP_LINE line;
		line.SizeOfStruct = sizeof(IMAGEHLP_LINE);

		if (SymGetLineFromAddr(hProcess, address, &offset, &line))
		{
			strncpy_s(sf.sourceFile, line.FileName, _TRUNCATE);
			sf.lineNumber = line.LineNumber;
		}
	}

	using namespace Rococo;

	static bool isReady = true;

	struct AllocatorInfo
	{
		std::unordered_map<size_t, size_t> allocationHistory;
		size_t totalMemoryAllocated = 0;
		size_t totalAllocations = 0;
		size_t outstandingAllocations = 0;
		size_t traceSize = -1;

		struct AllocUnit
		{
			enum { TRACE_DEPTH = 12 };
			size_t nBytes;
			DWORD64 callAddress[TRACE_DEPTH];
		};

		std::unordered_map<void*, AllocUnit> allocations;

		HANDLE hProcess;

		AllocatorInfo()
		{
			hProcess = GetCurrentProcess();

			SymInitialize(hProcess, NULL, TRUE);
			SymSetOptions(SYMOPT_LOAD_LINES);
		}

		void MarkAllocate(void* ptr, size_t nBytes)
		{
			outstandingAllocations++;
			totalAllocations++;
			totalMemoryAllocated += nBytes;

			auto i = allocationHistory.insert(std::make_pair(nBytes, 1));
			if (!i.second)
			{
				i.first->second++;
			}

			AllocUnit newUnit = { nBytes, {0} };

			if (nBytes == traceSize)
			{
				CONTEXT context;
				context.ContextFlags = CONTEXT_FULL;
				RtlCaptureContext(&context);

				STACKFRAME64 frame = { 0 };
				frame.AddrPC.Offset = context.Rip;
				frame.AddrPC.Mode = AddrModeFlat;
				frame.AddrFrame.Offset = context.Rbp;
				frame.AddrFrame.Mode = AddrModeFlat;
				frame.AddrStack.Offset = context.Rsp;
				frame.AddrStack.Mode = AddrModeFlat;

				int depthMin = 3;
				int depthMax = depthMin + AllocUnit::TRACE_DEPTH;

				int depth = 0;

				while (StackWalk(
					IMAGE_FILE_MACHINE_AMD64,
					hProcess,
					GetCurrentThread(),
					&frame,
					&context,
					nullptr,
					SymFunctionTableAccess64,
					SymGetModuleBase,
					nullptr
				))
				{
					depth++;
					if (depth < depthMin)
					{
						continue;
					}
					if (depth == depthMax)
					{
						break;
					}

					int index = depth - depthMin;

					WORD segment = frame.AddrPC.Segment;
					newUnit.callAddress[index] = frame.AddrPC.Offset;
				}
			}

			allocations[ptr] = newUnit;
		}

		void MarkFree(void* ptr)
		{
			outstandingAllocations--;
			allocations.erase(ptr);
		}

		typedef void (*FN_SEXY_MEMTRACE_CALLBACK)(DWORD64 address, const StackFrame& sf);

		void ReportTracedAllocations(uint32 maxResults, FN_SEXY_MEMTRACE_CALLBACK callback)
		{
			uint32 count = 0;

			if (traceSize == -1) return;

			for (const auto& i : allocations)
			{
				const AllocUnit& u = i.second;
				if (u.nBytes == traceSize)
				{
					for (DWORD64 address : u.callAddress)
					{
						if (address != 0)
						{
							StackFrame sf;
							GetFunctionName(hProcess, sf, address);
							callback(address, sf);
						}
					}			
				}

				// 0 indicates the end of the stack trace for the particular pointer
				callback(0, StackFrame{ 0 });

				if (++count == maxResults) break;
			}
		}

		~AllocatorInfo()
		{
			std::vector<std::pair<size_t, size_t>> items;
			for (auto i : allocationHistory)
			{
				items.push_back(i);
			}

			std::stable_sort(items.begin(), items.end(), [](const std::pair<size_t, size_t>& a, const std::pair<size_t, size_t>& b) { return a.first < b.first; });

		//	Rococo::OS::TripDebugger();
		}
	};

	AllocatorInfo allocatorInfo;
}
#endif

namespace
{
	struct DefaultSexyAllocator : Rococo::IAllocator
	{
		void* Allocate(size_t nBytes) override
		{
			void* buffer = malloc(nBytes);
#ifdef RECORD_ALLOCATION_HISTORY
			allocatorInfo.MarkAllocate(buffer, nBytes);		
#endif
			return buffer;
		}

		void FreeData(void* data) override
		{
#ifdef RECORD_ALLOCATION_HISTORY
			allocatorInfo.MarkFree(data);
#endif
			free(data);
		}

		void* Reallocate(void* ptr, size_t capacity) override
		{
			return realloc(ptr, capacity);
		}
	};

	static DefaultSexyAllocator defaultAllocator;
#ifdef RECORD_ALLOCATION_HISTORY
	void PrintFunctionData(DWORD64 address, const StackFrame& sf)
	{
		if (address == 0)
		{
			WriteToStandardOutput("\n");
			OutputDebugStringA("\n");
			return;
		}

		char buffer[2048];
		SafeFormat(buffer, "%p: %s %s (line %d)\n", address, sf.functionName, sf.sourceFile, sf.lineNumber);
		WriteToStandardOutput(buffer);
		OutputDebugStringA(buffer);
	}
#endif
}

namespace Rococo::Memory
{
	static IAllocator* globalSexyAllocator = &defaultAllocator;

	void SetSexyAllocator(IAllocator* allocator)
	{
		globalSexyAllocator = allocator == nullptr ? &defaultAllocator : allocator;
	}

	SEXYUTIL_API void* AllocateSexyMemory(size_t nBytes)
	{
		return globalSexyAllocator->Allocate(nBytes);
	}

	SEXYUTIL_API void FreeSexyMemory(void* buffer, size_t /* nBytes */)
	{
		globalSexyAllocator->FreeData(buffer);
	}

	SEXYUTIL_API void FreeSexyUnknownMemory(void* buffer)
	{
		globalSexyAllocator->FreeData(buffer);
	}

	SEXYUTIL_API IAllocator& GetSexyAllocator()
	{
		return *globalSexyAllocator;
	}

	SEXYUTIL_API void ValidateNothingAllocated()
	{
#ifdef RECORD_ALLOCATION_HISTORY
		if (allocatorInfo.outstandingAllocations != 0)
		{
			Rococo::OS::TripDebugger();
		}
#endif
	}

	SEXYUTIL_API size_t AllocationCount()
	{
#ifdef RECORD_ALLOCATION_HISTORY
		return allocatorInfo.outstandingAllocations;
#else
		return 0;
#endif
	}

	SEXYUTIL_API void MarkMemory(cstr message, cstr function, int line)
	{
#ifdef RECORD_ALLOCATION_HISTORY
		if (AllocationCount() == 0) return;

		char buffer[256];
		SafeFormat(buffer, "ALLOCATION_COUNT: %llu: %s @%s line %d\n", AllocationCount(), message, function, line);
		WriteToStandardOutput("%s", buffer);
		OutputDebugStringA(buffer);

		int j = 0;

		for (auto i : allocatorInfo.allocations)
		{
			SafeFormat(buffer, "\tOutstanding allocation: #%d: %p %llu bytes\n", j++, i.first, i.second.nBytes);
			WriteToStandardOutput("%s", buffer);
			OutputDebugStringA(buffer);

			if (j == 50 && allocatorInfo.outstandingAllocations > 4)
			{
				WriteToStandardOutput("...\n");
				OutputDebugStringA("...\n");
				break;
			}
		}

		allocatorInfo.ReportTracedAllocations(50, PrintFunctionData);
#else
		UNUSED(message);
		UNUSED(function);
		UNUSED(line);
#endif
	}

	SEXYUTIL_API void RecordAllocations(size_t nBytes)
	{

#ifdef RECORD_ALLOCATION_HISTORY
		allocatorInfo.traceSize = nBytes;
#else	
		UNUSED(nBytes);
#endif
	}
}
