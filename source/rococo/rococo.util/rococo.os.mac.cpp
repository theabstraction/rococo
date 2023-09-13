#include <sexy.types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/sysctl.h>
#include <rococo.io.h>
#define ROCOCO_USE_SAFE_V_FORMAT
#include <rococo.hashtable.h>
#include <errno.h>
#include <mach-o/dyld.h>
#include <mach/mach.h>
#include <mach/mach_time.h>
#include <unistd.h>
#include <os/lock.h>
#include <new>
#include <rococo.api.h>
#include <rococo.debugging.h>
#include <time.h>
#include <execinfo.h>
#include <cxxabi.h>
#include <rococo.allocators.h>
#include <vector>
#include <dlfcn.h>
#include <rococo.os.h>

namespace
{
   int breakFlags = 0;
   
#ifdef RASPBBERRY_NOODLES
   
   bool IsLittleEndian()
   {
	   union U
	   {
		   U(const int i): iValue(i) {}
		   const int iValue;
		   char cValues[4];
	   } u(1);

	   // little endian if true
	   return u.cValues[0] == 1;
   }
   
   struct UTF8
   {
	   std::vector<char> chararray;

       UTF8(const wchar_t* wsz)
       {
		   static_assert(sizeof(wchar_t) == 4, "!");
		   
           // OS X uses 32-bit wchar
           const int nChars = wcslen(wsz);
           // comp_bLittleEndian is in the lib I use in order to detect PowerPC/Intel
           CFStringEncoding encoding = IsLittleEndian() ? kCFStringEncodingUTF32LE : kCFStringEncodingUTF32BE;
           CFStringRef str = CFStringCreateWithBytesNoCopy(NULL, 
                                                          (const UInt8*)wsz, nChars * sizeof(wchar_t), 
                                                           encoding, false, 
                                                           kCFAllocatorNull
                                                           );

           const int bytesUtf8 = CFStringGetMaximumSizeOfFileSystemRepresentation(str);
           chararray.resize(bytesUtf8);
           CFStringGetFileSystemRepresentation(str, chararray.data(), bytesUtf8);
           CFRelease(str);
       }   

       operator const char*() const { return chararray.data(); }
   };
   
#else
   struct UTF8
   {
	   Rococo::U8FilePath path;
	   UTF8(const wchar_t* wsz)
	   {
		   Assign(path, wsz);
	   }
	   operator const char*() const { return path; }
   };
#endif
}

#define _stricmp strcasecmp

#include <ctime>

namespace Rococo
{
	FILE* _wfopen(const wchar_t* filename, const wchar_t* mode)
	{
		UTF8 u8Filename(filename);
		UTF8 u8Mode(mode);
		return fopen(u8Filename, u8Mode); 
	}
	
	void GetTimestamp(char str[26])
	{
		time_t t;
		time(&t);
		cstr buf = ctime(&t);
		strcpy(str, buf);
	}

   ThreadLock::ThreadLock()
   {
      static_assert(sizeof(implementation) >= sizeof(os_unfair_lock), "Increase backing store for OSSpinLock");
      auto* pLock = reinterpret_cast<os_unfair_lock*>(this->implementation);
      new (pLock) os_unfair_lock();
   }

   ThreadLock::~ThreadLock()
   {
      auto* pLock = reinterpret_cast<os_unfair_lock*>(this->implementation);
      pLock->~os_unfair_lock();
   }

   void ThreadLock::Lock()
   {
      auto* pLock = reinterpret_cast<os_unfair_lock*>(this->implementation);
      os_unfair_lock_lock(pLock);
   }

   void ThreadLock::Unlock()
   {
      auto* pLock = reinterpret_cast<os_unfair_lock*>(this->implementation);
      os_unfair_lock_unlock(pLock);
   }

   namespace IO
   {
      void UseBufferlessStdout()
      {
         setbuf(stdout, nullptr);
      }
   }

   namespace Debugging
   {
	   void FormatStackFrames(IStackFrameFormatter& formatter)
	   {
		   //  record stack trace upto 128 frames
		   int callstack[128] = {};

		   // collect stack frames
		   int  frames = backtrace((void**)callstack, 128);

		   // get the human-readable symbols (mangled)
		   char** strs = backtrace_symbols((void**)callstack, frames);

		   for (int i = 0; i < frames; ++i)
		   {
			   /*

				Typically this is how the backtrace looks like:

				0   <app/lib-name>     0x0000000100000e98 _Z5tracev + 72
				1   <app/lib-name>     0x00000001000015c1 _ZNK7functorclEv + 17
				2   <app/lib-name>     0x0000000100000f71 _Z3fn0v + 17
				3   <app/lib-name>     0x0000000100000f89 _Z3fn1v + 9
				4   <app/lib-name>     0x0000000100000f99 _Z3fn2v + 9
				5   <app/lib-name>     0x0000000100000fa9 _Z3fn3v + 9
				6   <app/lib-name>     0x0000000100000fb9 _Z3fn4v + 9
				7   <app/lib-name>     0x0000000100000fc9 _Z3fn5v + 9
				8   <app/lib-name>     0x0000000100000fd9 _Z3fn6v + 9
				9   <app/lib-name>     0x0000000100001018 main + 56
				10  libdyld.dylib      0x00007fff91b647e1 start + 0

				*/

				// split the string, take out chunks out of stack trace
				// we are primarily interested in module, function and address

			   char functionSymbol[1024] = {};
			   char moduleName[1024] = {};
			   int  offset = 0;
			   char addr[48] = {};

			   sscanf(strs[i], "%*s %1023s %47s %1023s %*s %d", moduleName, addr, functionSymbol, &offset);

			   StackFrame sf;
			   SafeFormat(sf.moduleName, sizeof(sf.moduleName), "%s", moduleName);
			   SafeFormat(sf.sourceFile, sizeof(sf.sourceFile), "");
			   sf.depth = i;
			   sf.lineNumber = offset;
			   sf.address.segment = 0;
			   sf.address.offset = atoll(addr);

			   int   validCppName = 0;
			   //  if this is a C++ library, symbol will be demangled
			   //  on success function returns 0
			   //
			   char* functionName = abi::__cxa_demangle(functionSymbol, NULL, 0, &validCppName);

			   if (validCppName == 0) // success
			   {
				   SafeFormat(sf.functionName, sizeof(sf.functionName), "%s", functionName);
			   }
			   else
			   {
				   //  in the above traceback (in comments) last entry is not
				   //  from C++ binary, last frame, libdyld.dylib, is printed
				   //  from here
				   SafeFormat(sf.functionName, sizeof(sf.functionName), "%s", functionSymbol);
			   }

			   formatter.Format(sf);

			   if (functionName)
			   {
				   free(functionName);
			   }
		   }
		   free(strs);
	   }
   }

   void memcpy_s(void *dest, size_t destSize, const void *src, size_t count)
   {
      memcpy(dest, src, count);
   }

   int sscanf_s(const char* buffer, const char* format, ...)
   {
      va_list args;
      va_start(args, format);
      return vsscanf(buffer, format, args);
   }

   int sprintf_s(char* buffer, size_t capacity, const char* format, ...)
   {
      va_list args;
      va_start(args, format);
      return vsnprintf(buffer, capacity, format, args);
   }

   int _vsnprintf_s(char* buffer, size_t capacity, size_t maxCount, const char* format, va_list args)
   {
      return vsnprintf(buffer, capacity, format, args);
   }

   void strcpy_s(char* dest, size_t capacity, const char* source)
   {
      strcpy(dest, source);
   }

   void strncpy_s(char* dest, size_t capacity, const char* source, size_t maxCount)
   {
      strncpy(dest, source, capacity);
   }

   void strcat_s(char* dest, size_t capacity, const char* source)
   {
      strcat(dest, source);
   }

   void strncat_s(char* dest, size_t capacity, const char* source, size_t maxCount)
   {
      strncpy(dest, source, maxCount);
   }
}

namespace Rococo
{
   bool FileModifiedArgs::Matches(cstr pingPath) const
   {
      const wchar_t* a = this->sysPath;
      cstr b = pingPath;
      if (*b == L'!') b++;

      while (*a != 0)
      {
		if (*a < 32 || *a > 126)
		{
			return false; // the unicode must be in the ascii range
		}
		if (*a == '/')
		{
			if (*b == '/')
			{
			   // dandy
			}
			else
			{
			   return false;
			}
		}
		else if (*a != *b) 
		{
			return false;
		}

		a++;
		b++;
      }

      return *b == 0;
  }
}

namespace Rococo
{
	namespace OS
	{
		bool isRunning = true;

		bool IsRunning()
		{
			return isRunning;
		}
		
		void SanitizePath(wchar_t* path)
		{
			for (auto* s = path; *s != 0; ++s)
			{
				if (*s == L'\\') *s = L'/';
			}
		}
		
		void SanitizePath(char* path)
		{
			for (char* s = path; *s != 0; ++s)
			{
				if (*s == '\\') *s = '/';
			}
		}

		void UILoop(uint32 milliseconds)
		{
			usleep(milliseconds * 1000);
		}

		void PrintDebug(const char* format, ...)
		{
#if _DEBUG
			va_list arglist;
			va_start(arglist, format);
			char line[4096];
			SafeVFormat(line, sizeof(line), format, arglist);
			printf("DBG: %s\n", line);
#endif
		}
		void Format_C_Error(int errorCode, char* buffer, size_t capacity)
		{
			strerror_r(errorCode, buffer, capacity);
		}

		int OpenForAppend(void** _fp, cstr name)
		{
			FILE** fp = (FILE**)_fp;
			*fp = fopen(name, "ab");
			return (*fp == nullptr) ? errno : 0;
		}

		int OpenForRead(void** _fp, cstr name)
		{
			FILE** fp = (FILE**)_fp;
			*fp = fopen(name, "rb");
			return (*fp == nullptr) ? errno : 0;
		}

		void ValidateBuild()
		{
			static_assert(sizeof(int64) == 8, "Bad int64");
			static_assert(sizeof(int32) == 4, "Bad int32");
			static_assert(sizeof(int16) == 2, "Bad int16");
			static_assert(sizeof(int8) == 1, "Bad int8");
		}

		bool IsDebugging()
		{
			int                 junk;
			int                 mib[4];
			struct kinfo_proc   info;
			size_t              size;

			// Initialize the flags so that, if sysctl fails for some bizarre 
			// reason, we get a predictable result.

			info.kp_proc.p_flag = 0;

			// Initialize mib, which tells sysctl the info we want, in this case
			// we're looking for information about a specific process ID.

			mib[0] = CTL_KERN;
			mib[1] = KERN_PROC;
			mib[2] = KERN_PROC_PID;
			mib[3] = getpid();

			// Call sysctl.

			size = sizeof(info);
			junk = sysctl(mib, sizeof(mib) / sizeof(*mib), &info, &size, NULL, 0);

			// We're being debugged if the P_TRACED flag is set.

			return ((info.kp_proc.p_flag & P_TRACED) != 0);
		}

		void TripDebugger()
		{
			__asm__ volatile("int $0x03");
			// __builtin_trap();;
		}

		void ToSysPath(char* path)
		{
			for (char* s = path; *s != 0; ++s)
			{
				if (*s == '\\') *s = '/';
			}
		}
		
		void ToSysPath(wchar_t* path)
		{
			for (auto * s = path; *s != 0; ++s)
			{
				if (*s == L'\\') *s = L'/';
			}
		}

		void ToUnixPath(char* path)
		{
			for (char* s = path; *s != 0; ++s)
			{
				if (*s == '\\') *s = '/';
			}
		}

		bool IsFileExistant(cstr filename)
		{
			int res = access(filename, R_OK);
			if (res < 0 && errno == ENOENT)
			{
				return false;
			}
			else
			{
				return true;
			}
		}
		
		bool IsFileExistant(const wchar_t* filename)
		{
			UTF8 u8Filename(filename);
			return IsFileExistant(u8Filename);
		}

		bool StripLastSubpath(char* fullpath)
		{
			int32 len = StringLength(fullpath);
			for (int i = len - 2; i > 0; --i)
			{
				if (fullpath[i] == (char) '/')
				{
					fullpath[i + 1] = 0;
					return true;
				}
			}

			return false;
		}

		ticks CpuTicks()
		{
			return mach_absolute_time();
		}

		ticks CpuHz()
		{
			mach_timebase_info_data_t theTimeBaseInfo;
			mach_timebase_info(&theTimeBaseInfo);

			return (ticks)(1.0e9 * ((double)theTimeBaseInfo.denom / (double)theTimeBaseInfo.numer));
		}

		ticks UTCTime()
		{
			static_assert(sizeof(time_t) == sizeof(ticks), "Need to fix UTCTime for OSX");
			time_t t = time(nullptr);
			return *(ticks*)&t;
		}

		void FormatTime(ticks utcTime, char* buffer, size_t nBytes)
		{
			char buf[32];
			ctime_r((time_t*)&utcTime, buf);

			SafeFormat(buffer, min(sizeof(buf), nBytes), "%s", buf);
		}

		MemoryUsage ProcessMemory()
		{
			return{ 0,0 };
		}

		bool DoesModifiedFilenameMatchResourceName(cstr modifiedFilename, cstr resourceName)
		{
			cstr p = modifiedFilename;
			cstr q = resourceName + 1;

			while (*p != 0)
			{
				if (*p != *q)
				{
					if (*q == '/')
					{
						// ok
					}
					else
					{
						return false;
					}
				}

				p++;
				q++;
			}

			return *q == 0;
		}

		void* AllocBoundedMemory(size_t nBytes)
		{
			return Rococo::Memory::_aligned_malloc(nBytes, 2048);
		}

		void FreeBoundedMemory(void* pMemory)
		{
			Rococo::Memory::_aligned_free(pMemory);
		}
		
		void SaveAsciiTextFile(TargetDirectory target, const wchar_t* filename, const fstring& text)
		{
			if (text.length > 1024_megabytes)
			{
				Throw(0, "Rococo::IO::SaveAsciiTextFile(%ls): Sanity check. String was > 1 gigabyte in length", filename);
			}

			U8FilePath fullpath;
			
			switch (target)
			{
				case TargetDirectory_UserDocuments:
				Format(fullpath, "/docs/%ls", filename);
				break;
			case TargetDirectory_Root:
				Assign(fullpath, filename);
				break;
			default:
				Throw(0, "Rococo::IO::SaveAsciiTextFile(... %ls): Unrecognized target directory", filename);
				break;
			}

			FILE* fp = fopen(fullpath, "wb");
			if (fp == nullptr)
			{
				Throw(0, "Cannot create file %s %s", fullpath.buf, strerror(errno));
			}
			
			auto bytesWritten = fwrite(text.buffer, 1, text.length, fp);
			fclose(fp);
		}
	} // OS
} // Rococo

namespace
{
	using namespace Rococo;

	typedef FILE* HANDLE;

	struct FileHandle
	{
		FILE* hFile;

		FileHandle(HANDLE _hFile) : hFile(_hFile)
		{
		}

		bool IsValid() const
		{
			return hFile != nullptr;
		}

		~FileHandle()
		{
			if (IsValid()) fclose(hFile);
		}

		operator HANDLE()
		{
			return hFile;
		}
	};

	void MakeContainerDirectory(wchar_t* filename)
	{
		int len = (int)wcslen(filename);

		for (int i = len - 2; i > 0; --i)
		{
			if (filename[i] == L'/')
			{
				filename[i + 1] = 0;
				return;
			}
		}
	}

	void GetContentDirectory(const wchar_t* contentIndicatorName, WideFilePath& content, IOS& os)
	{
		WideFilePath container;
		os.GetBinDirectoryAbsolute(container);

		MakeContainerDirectory(container.buf);

		WideFilePath indicatorFullPath;
		Format(indicatorFullPath, L"%s%s", container.buf, contentIndicatorName);
		if (!os.IsFileExistant(indicatorFullPath))
		{
			Throw(0, "Expecting content indicator %ls", indicatorFullPath.buf);
		}

		Format(content, L"%lsinstallation/", container.buf);
	}

	class Installation : public IInstallationSupervisor
	{
		IOS& os;
		WideFilePath contentDirectory;
		int32 len;
		stringmap<HString> macroToSubdir;
	public:
		Installation(const wchar_t* contentIndicatorName, IOS& _os) : os(_os)
		{
			GetContentDirectory(contentIndicatorName, contentDirectory, os);
			len = wcslen(contentDirectory.buf);
		}

		virtual ~Installation()
		{
		}

		void Free() override
		{
			delete this;
		}

		IOS& OS() override
		{
			return os;
		}

		const wchar_t* Content() const override
		{
			return contentDirectory;
		}

		void LoadResource(cstr resourcePath, ILoadEventsCallback& cb) override
		{
			if (resourcePath == nullptr || rlen(resourcePath) < 2) Throw(0, "OSX::LoadResource failed: <resourcePath> was blank");
			if (resourcePath[0] != '!') Throw(0, "OSX::LoadResource failed:\n%s\ndid not begin with ping '!' character", resourcePath);

			UTF8 u8content(contentDirectory);
			if (rlen(resourcePath) + rlen(u8content) >= _MAX_PATH)
			{
				Throw(0, "OSX::LoadResource failed: %ls%hs - filename was too long", contentDirectory.buf, resourcePath + 1);
			}

			if (strstr(resourcePath, "..") != nullptr)
			{
				Throw(0, "OSX::LoadResource failed: %hs - parent directory sequence '..' is forbidden", resourcePath);
			}

			Throw(0, "%s: Not implemented", __FUNCTION__);
		}

		void LoadResource(cstr resourcePath, IExpandingBuffer& buffer, int64 maxFileLength) override
		{
			if (resourcePath == nullptr || rlen(resourcePath) < 2) Throw(0, "OSX::LoadResource failed: <resourcePath> was blank");
			if (resourcePath[0] != '!') Throw(0, "OSX::LoadResource failed:\n%s\ndid not begin with ping '!' character", resourcePath);

			UTF8 u8content(contentDirectory);
			if (rlen(resourcePath) + rlen(u8content) >= _MAX_PATH)
			{
				Throw(0, "OSX::LoadResource failed: %ls%hs - filename was too long", contentDirectory.buf, resourcePath + 1);
			}

			if (strstr(resourcePath, "..") != nullptr)
			{
				Throw(0, "OSX::LoadResource failed: %hs - parent directory sequence '..' is forbidden", resourcePath);
			}

			WideFilePath absPath;
			Format(absPath, L"%ls%hs", contentDirectory.buf, resourcePath + 1);
			os.LoadAbsolute(absPath, buffer, maxFileLength);
		}

		bool TryLoadResource(cstr resourcePath, IExpandingBuffer& buffer, int64 maxFileLength) override
		{
			if (resourcePath == nullptr || rlen(resourcePath) < 2) Throw(0, "OSX::TryLoadResource failed: <resourcePath> was blank");
			if (resourcePath[0] != '!') Throw(0, "OSX::TryLoadResource failed:\n%s\ndid not begin with ping '!' character", resourcePath);

			UTF8 u8content(contentDirectory);
			if (rlen(resourcePath) + rlen(u8content) >= _MAX_PATH)
			{
				Throw(0, "OSX::TryLoadResource failed: %ls%hs - filename was too long", contentDirectory.buf, resourcePath + 1);
			}

			if (strstr(resourcePath, "..") != nullptr)
			{
				Throw(0, "OSX::TryLoadResource failed: %hs - parent directory sequence '..' is forbidden", resourcePath);
			}

			WideFilePath absPath;
			Format(absPath, L"%ls%hs", contentDirectory.buf, resourcePath + 1);

			if (!Rococo::IO::IsFileExistant(absPath))
			{
				return false;
			}

			os.LoadAbsolute(absPath, buffer, maxFileLength);

			return true;
		}

		bool DoPingsMatch(cstr a, cstr b) const override
		{
			try
			{
				WideFilePath sysPathA;
				ConvertPingPathToSysPath(a, sysPathA);

				WideFilePath sysPathB;
				ConvertPingPathToSysPath(b, sysPathB);

				return Eq(sysPathA, sysPathB);
			}
			catch (IException&)
			{
				return false;
			}
		}

		cstr GetFirstSlash(cstr path) const
		{
			for (cstr p = path + 1; *p != 0; p++)
			{
				if (*p == '/')
				{
					return p;
				}
			}

			return nullptr;
		}

		void ConvertPingPathToSysPath(cstr pingPath, WideFilePath& sysPath) const override
		{
			if (pingPath == nullptr || *pingPath == 0)
			{
				Throw(0, "Installation::ConvertPingPathToSysPath(...) Ping path was blank");
			}

			auto macroDir = "";
			const char* subdir = nullptr;

			if (*pingPath == '!')
			{
				subdir = pingPath + 1;

				Format(sysPath, L"%ls%hs", contentDirectory.buf, subdir);
			}
			else if (*pingPath == '#')
			{
				auto slash = GetFirstSlash(pingPath + 1);
				if (slash == nullptr)
				{
					Throw(0, "Installation::ConvertPingPathToSysPath(\"%s\"): expecting forward slash character in pingPath", pingPath);
				}

				subdir = slash + 1;

				char macro[IO::MAX_PATHLEN];
				memcpy_s(macro, sizeof(macro), pingPath, slash - pingPath);
				macro[slash - pingPath] = 0;

				auto i = macroToSubdir.find(macro);
				if (i == macroToSubdir.end())
				{
					Throw(0, "Installation::ConvertPingPathToSysPath(\"%s\"): unknown macro: %s", macro, pingPath);
				}

				macroDir = i->second.c_str();

				Format(sysPath, L"%ls%s%s", contentDirectory, macroDir + 1, subdir);
			}
			else
			{
				Throw(0, "Installation::ConvertPingPathToSysPath(\"%s\"): unknown prefix. Expecting ! or #", pingPath);
			}

			if (strstr(pingPath, "..") != nullptr)
			{
				Throw(0, "Installation::ConvertPingPathToSysPath(...) Illegal sequence in ping path: '..'");
			}

			IO::ToSysPath(sysPath.buf);
		}

		void ConvertSysPathToMacroPath(const wchar_t* sysPath, U8FilePath& pingPath, cstr macro) const override
		{
			U8FilePath fullPingPath;
			ConvertSysPathToPingPath(sysPath, fullPingPath);

			auto i = macroToSubdir.find(macro);
			if (i == macroToSubdir.end())
			{
				Throw(0, "Installation::ConvertSysPathToMacroPath(...) No macro defined: %s", macro);
			}

			cstr expansion = i->second.c_str();
			if (strstr(fullPingPath, expansion) == nullptr)
			{
				Throw(0, "Installation::ConvertSysPathToMacroPath(...\"%ls\", \"%s\") Path not prefixed by macro: %s", sysPath, macro, expansion);
			}

			Format(pingPath, "%hs/%hs", macro, fullPingPath.buf + i->second.length());
		}

		void ConvertSysPathToPingPath(const wchar_t* sysPath, U8FilePath& pingPath) const override
		{
			if (pingPath == nullptr || sysPath == nullptr) Throw(0, "ConvertSysPathToPingPath: Null argument");

			int sysPathLen = (int)wcslen(sysPath);

			size_t contentDirLength = wcslen(contentDirectory);

			if (0 != wcsncmp(sysPath, contentDirectory, wcslen(contentDirectory)))
			{
				Throw(0, "ConvertSysPathToPingPath: '%ls' did not begin with the content folder %ls", sysPath, contentDirectory.buf);
			}

			if (wcsstr(sysPath, L"..") != nullptr)
			{
				Throw(0, "ConvertSysPathToPingPath: '%ls' - Illegal sequence in ping path: '..'", sysPath);
			}

			Format(pingPath, "!%ls", sysPath + contentDirLength);

			IO::ToUnixPath(pingPath.buf);
		}

		void Macro(cstr name, cstr pingFolder) override
		{
			if (name == nullptr || *name != '#')
			{
				Throw(0, "Installation::Macro(name, ...): [name] did not begin with a hash '#' character");
			}

			if (pingFolder == nullptr || *pingFolder != '!')
			{
				Throw(0, "Installation::Macro(..., pingFolder): [pingFolder] did not begin with a ping '!' character");
			}

			U8FilePath pingRoot;
			int len = Format(pingRoot, "%s", pingFolder);
			IO::ToUnixPath(pingRoot.buf);
			if (pingRoot[len - 1] != '/')
			{
				Throw(0, "Installation::Macro(..., pingFolder): %s did not end with slash '/' character");
			}

			auto i = macroToSubdir.find(name);
			if (i != macroToSubdir.end())
			{
				Throw(0, "Installation::Macro(\"%s\", ...) - macro already defined", name);
			}

			macroToSubdir[name] = pingFolder;
		}

		bool TryExpandMacro(cstr macroPrefixPlusPath, U8FilePath& expandedPath) override
		{
			auto slash = GetFirstSlash(macroPrefixPlusPath + 1);
			if (slash == nullptr)
			{
				Throw(0, "Installation::TryExpandMacro(\"%s\"): expecting forward slash character in pingPath", macroPrefixPlusPath);
			}

			cstr subdir = slash + 1;

			U8FilePath macro;
			memcpy_s(macro.buf, macro.CAPACITY, macroPrefixPlusPath, slash - macroPrefixPlusPath);
			macro.buf[slash - macroPrefixPlusPath] = 0;

			auto i = macroToSubdir.find(macro.buf);
			if (i == macroToSubdir.end())
			{
				return false;
			}

			Format(expandedPath, "%s%s", i->second.c_str(), subdir);
			return true;
		}

		void CompressPingPath(cstr pingPath, U8FilePath& buffer) const override
		{
			// Implementation here is not optimal, but its okay for now, as this method is only used during initialization
			struct MacroToSubpath
			{
				HString macro;
				HString subpath;

				bool operator < (const MacroToSubpath& other) const
				{
					return other.macro.length() - other.subpath.length() > macro.length() - subpath.length();
				}
			};

			std::vector<MacroToSubpath> macros;
			for (auto& i : macroToSubdir)
			{
				macros.push_back({ HString(i.first), i.second });
			}

			std::sort(macros.begin(), macros.end()); // macros is now sorted in order of macro length

			for (auto& m : macros)
			{
				if (StartsWith(pingPath, m.subpath))
				{
					Format(buffer, "%hs/%hs", m.macro.c_str(), pingPath + m.subpath.length());
					return;
				}
			}

			Format(buffer, "%hs", pingPath);
		}
	};

	bool TryGetDllFileName(U8FilePath& filename)
	{
		Dl_info info;
		int result = dladdr((const void*)TryGetDllFileName, &info);
		if (result == 0) return false;
		Format(filename, "%s", info.dli_fname);
		return true;
	}

	class OSX : public IOSSupervisor
	{
		WideFilePath binDirectory;
		IEventCallback<SysUnstableArgs>* onUnstable;
	public:
		OSX() : onUnstable(nullptr)
		{
			U8FilePath u8Bin;
			uint32 lcapacity = u8Bin.CAPACITY;

			if (!TryGetDllFileName(u8Bin))
			{
				if (_NSGetExecutablePath(u8Bin.buf, &lcapacity) != 0)
				{
					Throw(0, "dladdr and _NSGetExecutablePath failed");
				}
			}

			Format(binDirectory, L"%s", u8Bin.buf);

			MakeContainerDirectory(binDirectory.buf);
		}

		void Free() override
		{
			delete this;
		}

		void Monitor(const wchar_t* absPath) override
		{
			// Not supported on the Max. Develope on a PC, a proper computer, then copy and paste to your noddy MAC
		}

		void EnumerateModifiedFiles(IEventCallback<FileModifiedArgs>& cb) override
		{
		}

		void FireUnstable() override
		{
			SysUnstableArgs unused;
			if (onUnstable) onUnstable->OnEvent(unused);
		}

		void SetUnstableHandler(IEventCallback<SysUnstableArgs>* cb) override
		{
			onUnstable = cb;
		}

		bool IsFileExistant(const wchar_t* absPath) const override
		{
			return IO::IsFileExistant(absPath);
		}

		void ConvertUnixPathToSysPath(const wchar_t* unixPath, WideFilePath& sysPath) const override
		{
			Format(sysPath, L"%s", unixPath);
		}

		void LoadAbsolute(const wchar_t* absPath, IExpandingBuffer& buffer, int64 maxFileLength) const override
		{
			UTF8 u8AbsPath(absPath);
			FileHandle hFile = fopen(u8AbsPath, "r");
			if (!hFile.IsValid()) Throw(errno, "%s failed: Error opening file %ls", __FUNCTION__, absPath);

			fseek(hFile, 0, SEEK_END);
			long length = ftell(hFile);

			if ((int64)length > maxFileLength)
			{
				Throw(0, "%s failed: File <%ls> was too large at over %lld bytes", __FUNCTION__, absPath, maxFileLength);
			}

			fseek(hFile, 0, SEEK_SET);

			buffer.Resize((size_t)length);

			int64 bytesLeft = (int64)length;
			ptrdiff_t offset = 0;

			uint8* data = (uint8*)buffer.GetData();

			while (bytesLeft > 0)
			{
				uint64 chunk = (uint64)min(bytesLeft, 65536LL);
				size_t nBytesRead = fread(data + offset, 1, chunk, hFile);

				if (nBytesRead != chunk)
				{
					Throw(0, "%s: Error reading file <%ls>. Failed to read chunk", __FUNCTION__, absPath);
				}

				offset += (ptrdiff_t)chunk;
				bytesLeft -= (int64)chunk;
			}
		}

		void GetBinDirectoryAbsolute(WideFilePath& output) const override
		{
			output = this->binDirectory;
		}

		size_t MaxPath() const override
		{
			return _MAX_PATH;
		}
	};
}

namespace Rococo::IO
{
	IOSSupervisor* GetIOS()
	{
		return new OSX();
	}

	IInstallationSupervisor* CreateInstallation(const wchar_t* contentIndicatorName, IOS& os)
	{
		return new Installation(contentIndicatorName, os);
	}
}

namespace Rococo::Windows
{
	IWindow& NoParent()
	{
		struct NullWindow : public IWindow
		{
			virtual operator ID_OSWINDOW() const
			{
				return ID_OSWINDOW::Invalid();
			}
		};

		static NullWindow nullWindow;
		return nullWindow;
	}
}


namespace Rococo::OS
{
	void SaveAsciiTextFile(TargetDirectory target, cstr filename, const fstring& text)
	{
		if (text.length > 1024_megabytes)
		{
			Throw(0, "Rococo::IO::SaveAsciiTextFile(%s): Sanity check. String was > 1 gigabyte in length", filename);
		}

		switch (target)
		{
		case TargetDirectory_UserDocuments:
			{
				char fullpath[Rococo::IO::MAX_PATHLEN];
			
				SafeFormat(fullpath, Rococo::IO::MAX_PATHLEN, "~/%s", filename);

				FILE* fp = fopen(fullpath, "wb");

				struct FPCLOSER
				{
					FILE* fp;

					~FPCLOSER()
					{
						fclose(fp);
					}
				} fpCloser { fp };

				size_t writeSize = fwrite(text, 1, text.length, fp);

				if (writeSize != (size_t) text.length)
				{
					Throw(errno, "Rococo::IO::SaveAsciiTextFile(%s) : failed to write text to file", filename);
				}
			}
			break;
		default:
			Throw(0, "Rococo::IO::SaveAsciiTextFile(... %s): Unrecognized target directory", filename);
			break;
		}
	}
	
	void FormatErrorMessage(char* message, size_t sizeofBuffer, int errorCode)
	{
		SafeFormat(message, sizeofBuffer, "%s", strerror(errorCode));
	}
} // Rococo::OS

#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

template<class CHAR> const CHAR* GetFirstNullChar(const CHAR* s)
{
	auto i = s;
	while (*i != 0) i++;
	return i;
}

static bool IsFile(cstr path)
{
	struct stat s;
	int result = stat(path, &s);

	return (result == 0 && s.st_mode & S_IFREG) != 0;
}

namespace Rococo::IO
{
	void ForEachFileInDirectory(const wchar_t* directory, Rococo::IEventCallback<Rococo::IO::FileItemData>& callback,  bool recurse)
	{
		if (recurse) Throw(0, "ForEachFileInDirectory recurse not implemented on OSX");
		
		struct Anon
		{
			DIR *d = nullptr;
			U8FilePath qualifiedPath;

			Anon(cstr directory)
			{
				bool isSlashed = GetFirstNullChar(directory)[-1] == L'/';

				Format(qualifiedPath, "%s%s", directory, isSlashed ? "" : "/");

				d = opendir(qualifiedPath);
				if (d == nullptr)
				{
					Throw(errno, "Could not enumerate files in %s", directory);
				}
			}

			void Run(Rococo::IEventCallback<Rococo::IO::FileItemData>& callback)
			{
				while(true)
				{
					dirent *entry = readdir(d);
					if (entry == nullptr) break;
					
#ifdef _DIRENT_HAVE_D_TYPE
					if (entry->d_type == DT_REG) // regular file
#else
					U8FilePath fullPath;
					Format(fullPath, "%s%s", qualifiedPath.buf, entry->d_name);				
					if (IsFile(fullPath))
#endif
					{
						WideFilePath wPath;
						Assign(wPath, fullPath);
						
						WideFilePath containerRelRoot;
						Format(containerRelRoot, L"%hs", qualifiedPath.buf);
						
						WideFilePath itemRelContainer;
						Format(itemRelContainer, L"%hs", entry->d_name);
							
						Rococo::IO::FileItemData fid = 
						{
							wPath,
							containerRelRoot.buf,
							itemRelContainer.buf,
							false
						};
						
						callback.OnEvent(fid);
					}
				}
			}

			~Anon()
			{
				if (d != nullptr)
				{
					closedir(d);
				}
			}
		};
		
		U8FilePath u8Dir;
		Assign(u8Dir, directory);
		
		Anon filesearch(u8Dir);
		filesearch.Run(callback);
	}
}

#include <stdio.h>

namespace
{
	struct ROBIN: Rococo::IO::IReadOnlyBinaryMapping
	{
		std::vector<char> data;
		
		ROBIN(const wchar_t* sysPath)
		{
			U8FilePath u8SysPath;
			Assign(u8SysPath, sysPath);
			auto* fp = fopen(u8SysPath, "rb");
			if (fp == nullptr)
			{
				Throw(errno, "Could not open binary file %hs", u8SysPath);
			}
			
			struct AUTOFP
			{
				FILE* fp;
				~AUTOFP() { fclose(fp); }
			} FP {fp};
			
			fseek(fp, 0, SEEK_END);
			auto len = ftell(fp);
			fseek(fp, 0, SEEK_SET);
			
			if (len > 1024_megabytes)
			{
				Throw(errno, "Cannot handle binary mappings larger than 1GB. (%hs)", u8SysPath);
			}
			
			data.resize(len);
			
			auto dataLeft = len;
			auto* cursor = data.data();
			
			for(;;)
			{
				size_t blocksize = min<size_t>(dataLeft, 8192);
				
				if (blocksize == 0)
				{
					break;
				}
				
				size_t bytesread = fread(cursor, 1, blocksize, fp);
				
				if (bytesread == 0)
				{
					if (feof(fp))
					{
						return;
					}
					else
					{
						Throw(0, "Error reading file: %s", u8SysPath);
					}
				}
				
				cursor += bytesread;
				dataLeft -= bytesread;
			}
		}
		
		const char* Data() const override
		{
			return data.data();
		}
		
		const uint64 Length() const override
		{
			return data.size();
		}
		
		void Free() override
		{
			delete this;
		}
	};
}

namespace Rococo::IO
{
	IReadOnlyBinaryMapping* CreateReadOnlyBinaryMapping(const wchar_t* sysPath)
	{
		return new ROBIN(sysPath);
	}
}