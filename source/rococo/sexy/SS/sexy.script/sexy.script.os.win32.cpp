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

#include "sexy.script.stdafx.h"

#ifdef char_IS_WIDE
# define _UNICODE
# define UNICODE
#endif

#include <rococo.win32.target.win7.h>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "sexy.debug.types.h"
#include "sexy.compiler.public.h"
#include "sexy.script.h"
#include <memory>
#include <algorithm>

#define ROCOCO_USE_SAFE_V_FORMAT
#include <rococo.strings.h>
#include <rococo.api.h>
#include <rococo.os.h>

#define BREAK_ON_THROW

#ifndef SCRIPT_IS_LIBRARY
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	return TRUE;
}
#endif

namespace
{
	void CloseAndThrowOS(HANDLE toClose, const char* message, const char* filename)
	{
      CloseHandle(toClose);
		Rococo::Throw(GetLastError(), ("%s: %s"), message, filename);
	}
}

namespace Rococo {
	namespace OS
	{
		bool TryGetDefaultNativeSrcDir(wchar_t* data, size_t capacity)
		{
			while (StripLastSubpath(data))
			{
				wchar_t fullpath[_MAX_PATH];
				SafeFormat(fullpath, _MAX_PATH, L"%s%s", data, L"src_indicator.txt");
				if (IsFileExistant(fullpath))
				{
					StringCat(data, L"content\\scripts\\native\\", (int32)capacity);
					return true;
				}
			}

			return false;
		}

		void GetDefaultNativeSrcDir(wchar_t* data, size_t capacity)
		{
#ifdef SCRIPT_IS_LIBRARY
			GetCurrentDirectoryW((DWORD)capacity, data);
#else
			HMODULE hModule = GetModuleHandle(nullptr);
			if (!hModule)
			{
				Throw(GetLastError(), "SEXY_NATIVE_SRC_DIR. Failed to get default variable: cannot get module handle for the sexy script dynamic link library");
			}

			GetModuleFileNameW(hModule, data, (DWORD)capacity);
#endif
			if (TryGetDefaultNativeSrcDir(data, capacity))
			{
				return;
			}

			GetCurrentDirectoryW((DWORD)capacity, data);

			if (TryGetDefaultNativeSrcDir(data, capacity))
			{
				return;
			}
			
			Rococo::Throw(GetLastError(), "SEXY_NATIVE_SRC_DIR. Failed to get default variable: cannot find src_indicator.txt descending from sexy.script.dll");
		}

		void GetEnvVariable(wchar_t* data, size_t capacity, const wchar_t* envVariable)
		{
			if (0 == GetEnvironmentVariableW(envVariable, data, (DWORD)capacity))
			{
				if (Eq(envVariable, L"SEXY_NATIVE_SRC_DIR"))
				{
					GetDefaultNativeSrcDir(data, capacity);

					if (INVALID_FILE_ATTRIBUTES == GetFileAttributesW(data))
					{
						Rococo::Throw(GetLastError(), "Error associating environment variable %ws with the sexy native source directory %ws", envVariable, data);
					}

					SetEnvironmentVariableW(envVariable, data);
					return;
				}

				Throw(GetLastError(), ("Environment variable %ws not found"), envVariable);
			}
		}

		typedef void(*FN_AddNativeSexyCalls)(Rococo::Script::IScriptSystem& ss);

		Rococo::Script::FN_CreateLib GetLibCreateFunction(const wchar_t* dynamicLinkLibOfNativeCalls, bool throwOnError)
		{
			WideFilePath linkLib;
			Format(linkLib, L"%ls.dll", dynamicLinkLibOfNativeCalls);
			HMODULE lib = LoadLibraryW(linkLib);
			if (lib == nullptr)
			{
				if (throwOnError) Rococo::Throw(GetLastError(), "Could not load %ls", linkLib.buf);
				return nullptr;
			}

			FARPROC fp = GetProcAddress(lib, "CreateLib");
			if (fp == nullptr)
			{
				if (throwOnError) Rococo::Throw(GetLastError(), "Could not find  INativeLib* CreateLib(...) in %ls", linkLib.buf);
				return nullptr;
			}

			Rococo::Script::FN_CreateLib createFn = (Rococo::Script::FN_CreateLib) fp;
			return createFn;
		}

		Rococo::Script::FN_CreateLib GetLibCreateFunction(cstr origin, const void* inMemoryDLL, int32 nBytesLength)
		{
			if (nBytesLength <= 0)
			{
				Throw(0, "Cannot create DLL %s\nBad length %d\n", origin, nBytesLength);
			}

			if (nBytesLength > 512_megabytes)
			{
				Throw(0, "Cannot create DLL %s\nExceeded maximum size of 512 MB\n", origin);
			}

			DWORD uint32Length = (DWORD)nBytesLength;

			U8FilePath tempPath;
			if (0 == GetTempPathA(tempPath.CAPACITY, tempPath.buf))
			{
				Throw(GetLastError(), "Could not open temporary path");
			}

			U8FilePath tempFile;
			Format(tempFile, "%hs%hs", tempPath.buf, origin);

			struct AutoFile
			{
				HANDLE hFile;
				~AutoFile()
				{
					if (hFile != INVALID_HANDLE_VALUE)
					{
						CloseHandle(hFile);
					}
				}
				void Close()
				{
					CloseHandle(hFile);
					hFile = INVALID_HANDLE_VALUE;
				}
				operator HANDLE() { return hFile; }
			} hFile = {
				CreateFile(tempFile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL)
			};

			if (hFile == INVALID_HANDLE_VALUE)
			{
				Throw(GetLastError(), "Could not open file for writing: %hs", tempFile.buf);
			}

			DWORD bytesWritten = 0;
			if (!WriteFile(hFile, inMemoryDLL, uint32Length, &bytesWritten, nullptr)
				|| bytesWritten != uint32Length)
			{
				Throw(GetLastError(), "Could not write %u bytes to %hs", uint32Length, tempFile.buf);
			}

			hFile.Close();

			HMODULE lib = LoadLibrary(tempFile);
			if (lib == nullptr)
			{
				Rococo::Throw(GetLastError(), "Could not load %hs", origin);
			}

			FARPROC fp = GetProcAddress(lib, "CreateLib");
			if (fp == nullptr)
			{
				Rococo::Throw(GetLastError(), "Could not find  INativeLib* CreateLib(...) in %hs", origin);
			}

			Rococo::Script::FN_CreateLib createFn = (Rococo::Script::FN_CreateLib) fp;
			return createFn;
		}
	}
} // Rococo::OS

#define ROCOCO_MEMORY_MANAGEMENT
#ifdef ROCOCO_MEMORY_MANAGEMENT
#include <rococo.allocators.inl>

namespace Rococo::Memory
{
	template<class T>
	class ScriptTrackingAllocator
	{
	public:
		size_t totalFreed = 0;
		AllocatorMetrics stats;

		struct TrackingString
		{
			char msg[256];
			int count;
		};

		struct BigTrackingAtom : TrackingAtom
		{
			Debugging::StackFrame::Address address;
		};

		std::unordered_map<size_t, BigTrackingAtom, hash_size_t, size_t_equal_to, std_Malloc_Allocator<std::pair<const size_t, BigTrackingAtom>>> tracking;

		using TWatch = std::unordered_map<size_t, TrackingString, hash_size_t, size_t_equal_to, std_Malloc_Allocator<std::pair<const size_t, TrackingString>>>;
		using TMetrics = std::unordered_map<size_t, size_t, hash_size_t, size_t_equal_to, std_Malloc_Allocator<std::pair<const size_t, size_t>>>;

		TMetrics metrics;
		TWatch stackWatch;
		size_t countWatched = 0;

		enum {ALLOCATION_SIZE_WATCHED = 192};

		void AddMetrics(size_t nBytes)
		{
			auto i = metrics.find(nBytes);
			if (i == metrics.end())
			{
				metrics.insert(std::make_pair(nBytes, 1));
			}
			else
			{
				i->second++;
			}
		}

		void ShowMetrics()
		{
			auto* alloc_printf = Rococo::Debugging::Log;

			struct Metric
			{
				size_t allocSize;
				size_t allocCount;
				size_t product;
			};

			struct MetricSort
			{
				bool operator()(const Metric& a, const Metric& b) const
				{
					return a.product < b.product;
				}
			} byProduct; 

			std::vector<Metric> metricArray;
			for (auto i : metrics)
			{
				Metric m;
				m.allocSize = i.first;
				m.allocCount = i.second;
				m.product = m.allocSize * m.allocCount;
				metricArray.push_back(m);
			}

			std::sort(metricArray.begin(), metricArray.end(), byProduct);
			
			alloc_printf("Metrics:\n");

			for (auto& m : metricArray)
			{
				alloc_printf(" %8llu nBytes x %8llu = %8llu\n", m.allocSize, m.allocCount, m.product);
			}
		}

		void AddProblemTracker(size_t nBytes)
		{
			if (nBytes == ALLOCATION_SIZE_WATCHED)
			{
				countWatched++;

				enum { DEPTH = 5 };
				
				TrackingString ts;
				auto address = Debugging::FormatStackFrame(ts.msg, sizeof ts.msg, DEPTH);
				Rococo::Debugging::Log(" Problem -> %s\n", ts.msg);

				/* if (strstr(ts.msg, "NastyBugString") != nullptr)
				{
					TrackingString helper;
					FormatStackFrame(helper.msg, sizeof helper.msg, DEPTH + 1);
					Rococo::Debugging::Log(" NastyBugString -> %s\n", helper.msg);

				}
				*/
				
				auto i = stackWatch.find(address.offset);
				if (i == stackWatch.end())
				{
					stackWatch.insert(std::make_pair((const size_t) address.offset, ts));
				}
				else
				{
					i->second.count++;
				}
			}
		}

		void* ModuleAllocate(std::size_t nBytes)
		{
			using namespace Rococo::Debugging;
			stats.totalAllocationSize += nBytes;
			stats.totalAllocations++;

			AddProblemTracker(nBytes);

			auto* data = malloc(nBytes);
			if (!data)
			{
				stats.failedAllocations++;
				Rococo::Throw(0, "%s(%s): Cannot allocate %llu bytes", __FUNCTION__, nBytes);
			}

			AddTrackingData(data, nBytes, false);
		//	AddMetrics(nBytes);

			return data;
		}

		void AddTrackingData(void* buffer, size_t nBytes, bool addPCAddresses)
		{
			enum { TRACK_DEPTH = 4};

			Debugging::StackFrame::Address address = addPCAddresses ? Debugging::FormatStackFrame(nullptr, 0, TRACK_DEPTH) : Debugging::StackFrame::Address { 0, 0 };

			auto i = tracking.find((size_t)buffer);
			if (i == tracking.end())
			{
				tracking.insert(std::make_pair((const size_t)buffer, BigTrackingAtom{ nBytes, false, 0, address }));
			}
			else
			{
				BigTrackingAtom& atom = i->second;
				atom.address = address;
				atom.bufferLength = nBytes;
				atom.reuseCount++;
				atom.wasFreed = false;
			}
		}

		void ModuleFree(void* buffer)
		{
			stats.totalFrees++;

			if (buffer)
			{
				auto i = tracking.find((size_t)buffer);
				if (i != tracking.end())
				{
					TrackingAtom& atom = i->second;

					if (!atom.wasFreed)
					{
						totalFreed += atom.bufferLength;
						atom.wasFreed = true;
					}
				}
				else
				{
					// This element should not have been freed with this allocator
					OS::TripDebugger();
				}

				stats.usefulFrees++;
				free(buffer);
			}
			else
			{
				stats.blankFrees++;
			}
		}

		void Log(cstr name, cstr intro)
		{
			auto* allocator_printf = Rococo::Debugging::Log;
			Rococo::Memory::Log(stats, name, intro, allocator_printf);

			std::unordered_map<size_t, size_t, hash_size_t, size_t_equal_to, std_Malloc_Allocator<std::pair<const size_t, size_t>>> leakMapSizeToCount;

			for (auto i : tracking)
			{
				BigTrackingAtom& atom = i.second;
				if (!atom.wasFreed)
				{
					std::pair<const size_t, size_t> newItem(atom.bufferLength, 1);
					auto j = leakMapSizeToCount.insert(newItem);
					if (!j.second)
					{
						j.first->second++;
					}

					atom.address;
					char buffer[256];
					Debugging::FormatStackFrame(buffer, sizeof buffer, atom.address);
					allocator_printf("Leak at %p:\n\t%s\n", i.first, buffer);
				}
			}

			if (!leakMapSizeToCount.empty())
			{
				allocator_printf(" Leaks detected: (%llu bytes)\n", stats.totalAllocationSize - totalFreed);
				for (auto i : leakMapSizeToCount)
				{
					allocator_printf("%9llu bytes x %-9llu = %llu bytes\n", i.first, i.second, i.first * i.second);
				}
			}
			else
			{
				allocator_printf("No leaks detected. Keep up the good programming work!\n\n");

				ShowMetrics();
			}

			if constexpr (ALLOCATION_SIZE_WATCHED != 0)
			{
				allocator_printf("%llu byte allocation sizes are being watched. Total: %llu\n\n", ALLOCATION_SIZE_WATCHED, countWatched);

				for (auto& i : stackWatch)
				{
					TrackingString& ts = i.second;
					allocator_printf("%lld x %s\n", ts.count, ts.msg);
				}
			}

			allocator_printf("\n\n");
		}

		using TDefaultMonitor = AllocatorMonitor<T>;
	};
}

DeclareAllocator(ScriptTrackingAllocator, SexyScript, g_allocator)
Rococo::Memory::AllocatorMonitor<SexyScript> monitor;

OVERRIDE_MODULE_ALLOCATORS_WITH_FUNCTOR(g_allocator)
#else
_NODISCARD _Ret_notnull_ _Post_writable_byte_size_(nBytes) _VCRT_ALLOCATOR
void* __CRTDECL operator new(std::size_t nBytes)
{
	return Rococo::Memory::GetSexyAllocator().Allocate(nBytes);
}

_Ret_maybenull_ _Success_(return != NULL) _Post_writable_byte_size_(nBytes) _VCRT_ALLOCATOR
void* __CRTDECL operator new(size_t nBytes, ::std::nothrow_t const&) noexcept
{
	try
	{
		return Rococo::Memory::GetSexyAllocator().Allocate(nBytes);
	}
	catch (...)
	{
		return nullptr;
	}
}

_NODISCARD _Ret_notnull_ _Post_writable_byte_size_(nBytes) _VCRT_ALLOCATOR
void* __CRTDECL operator new[](size_t nBytes)
{
	return Rococo::Memory::GetSexyAllocator().Allocate(nBytes);
}

_NODISCARD _Ret_maybenull_ _Success_(return != NULL) _Post_writable_byte_size_(nBytes) _VCRT_ALLOCATOR
void* __CRTDECL operator new[](size_t nBytes, ::std::nothrow_t const&) noexcept
{
	try
	{
		return Rococo::Memory::GetSexyAllocator().Allocate(nBytes);
	}
	catch (...)
	{
		return nullptr;
	}
}

void operator delete(void* buffer) throw()
{
	Rococo::Memory::GetSexyAllocator().FreeData(buffer);
}
#endif