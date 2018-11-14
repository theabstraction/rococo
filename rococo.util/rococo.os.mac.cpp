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
#include <rococo.strings.h>
#include <errno.h>
#include <mach-o/dyld.h>
#include <CoreServices/CoreServices.h>
#include <mach/mach.h>
#include <mach/mach_time.h>
#include <unistd.h>
#include <os/lock.h>
#include <new>
#include <rococo.api.h>
#include <rococo.debugging.h>

#include <unordered_map>
#include <string>

#include <time.h>

#include <execinfo.h>
#include <cxxabi.h>

namespace
{
   int breakFlags = 0;
}

int _stricmp(const char* a, const char* b)
{
	const char* p = a;
	const char* q = b;

	for (; *p != 0 && *p != 0; p++, q++)
	{
		int P = (unsigned char) *p;
		int Q = (unsigned char) *q;
		int diff = P - Q;
		if (diff) return diff;
	}

	int P = (unsigned char)*p;
	int Q = (unsigned char)*q;
	return P - Q;
}

namespace Rococo
{
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
   bool FileModifiedArgs::Matches(cstr resource) const
   {
      cstr a = this->resourceName;
      cstr b = resource;
      if (*b == L'!') b++;

      while (*a != 0)
      {
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
         else if (*a != *b) return false;

         a++;
         b++;
      }

      return *b == 0;
   }

   void FileModifiedArgs::GetPingPath(rchar* path, size_t capacity) const
   {
      SafeFormat(path, capacity, "!%s", resourceName);
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
		void Format_C_Error(int errorCode, rchar* buffer, size_t capacity)
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

		bool StripLastSubpath(rchar* fullpath)
		{
			int32 len = StringLength(fullpath);
			for (int i = len - 2; i > 0; --i)
			{
				if (fullpath[i] == (rchar) '/')
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

   void MakeContainerDirectory(rchar* filename)
   {
      int len = (int)rlen(filename);

      for (int i = len - 2; i > 0; --i)
      {
         if (filename[i] == '/')
         {
            filename[i + 1] = 0;
            return;
         }
      }
   }

   struct FilePath
   {
      rchar data[_MAX_PATH];
      operator rchar*() { return data; }
   };

   void GetContentDirectory(cstr contentIndicatorName, FilePath& path, IOS& os)
   {
      FilePath binDirectory;
      os.GetBinDirectoryAbsolute(binDirectory, os.MaxPath());

      path = binDirectory;

      if (strstr(contentIndicatorName, "/") != nullptr)
      {
         // The indicator is part of a path
         if (os.IsFileExistant(contentIndicatorName))
         {
            SecureFormat(path.data, sizeof(path.data), "%s", contentIndicatorName);
            MakeContainerDirectory(path);
            return;
         }
      }

      size_t len = rlen(path);

      while (len > 0)
      {
         FilePath indicator;
         StackStringBuilder sb(indicator.data, sizeof(indicator.data));
         sb.AppendFormat("%s%s", path.data, contentIndicatorName);
         if (os.IsFileExistant(indicator))
         {
            sb << "content/";
            return;
         }

         MakeContainerDirectory(path);

         size_t newLen = rlen(path);
         if (newLen >= len) break;
         len = newLen;
      }

      Throw(0, "Could not find %s below the executable folder '%s'", contentIndicatorName, binDirectory);
   }

   class Installation : public IInstallationSupervisor
   {
      IOS& os;
      FilePath contentDirectory;
	  int32 len;
	  std::unordered_map<std::string, std::string> macroToSubdir;
   public:
      Installation(cstr contentIndicatorName, IOS& _os) : os(_os)
      {
         GetContentDirectory(contentIndicatorName, contentDirectory, os);
		 len = strlen(contentDirectory.data);
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

      const fstring Content() const override
      {
		  return fstring{ contentDirectory.data, len };
      }

      void LoadResource(cstr resourcePath, IExpandingBuffer& buffer, int64 maxFileLength) override
      {
         if (resourcePath == nullptr || rlen(resourcePath) < 2) Throw(0, "OSX::LoadResource failed: <resourcePath> was blank");
         if (resourcePath[0] != '!') Throw(0, "OSX::LoadResource failed:\n%s\ndid not begin with ping '!' character", resourcePath);

         if (rlen(resourcePath) + rlen(contentDirectory) >= _MAX_PATH)
         {
            Throw(0, "OSX::LoadResource failed: %s%s - filename was too long", contentDirectory, resourcePath + 1);
         }

         if (strstr(resourcePath, "..") != nullptr)
         {
            Throw(0, "OSX::LoadResource failed: %s - parent directory sequence '..' is forbidden", resourcePath);
         }

         FilePath sysPath;
         os.ConvertUnixPathToSysPath(resourcePath + 1, sysPath, _MAX_PATH);

         FilePath absPath;
         SecureFormat(absPath.data, sizeof(absPath), "%s%s", contentDirectory.data, sysPath.data);

         os.LoadAbsolute(absPath, buffer, maxFileLength);
      }

	  bool DoPingsMatch(cstr a, cstr b) const override
	  {
		  try
		  {
			  char sysPathA[IO::MAX_PATHLEN];
			  ConvertPingPathToSysPath(a, sysPathA, IO::MAX_PATHLEN);

			  char sysPathB[IO::MAX_PATHLEN];
			  ConvertPingPathToSysPath(b, sysPathB, IO::MAX_PATHLEN);

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

	  void ConvertPingPathToSysPath(cstr pingPath, char* sysPath, size_t sysPathCapacity) const override
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

			  int fulllen = SecureFormat(sysPath, sysPathCapacity, "%s%s", contentDirectory.data, subdir);
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

			  SecureFormat(sysPath, sysPathCapacity, "%s%%ss", contentDirectory.data, macroDir + 1, subdir);
		  }
		  else
		  {
			  Throw(0, "Installation::ConvertPingPathToSysPath(\"%s\"): unknown prefix. Expecting ! or #", pingPath);
		  }

		  if (strstr(pingPath, "..") != nullptr)
		  {
			  Throw(0, "Installation::ConvertPingPathToSysPath(...) Illegal sequence in ping path: '..'");
		  }

		  OS::ToSysPath(sysPath);
	  }

	  void ConvertSysPathToMacroPath(cstr sysPath, char* pingPath, size_t pingPathCapacity, cstr macro) const override
	  {
		  char fullPingPath[IO::MAX_PATHLEN];
		  ConvertSysPathToPingPath(sysPath, fullPingPath, IO::MAX_PATHLEN);

		  auto i = macroToSubdir.find(macro);
		  if (i == macroToSubdir.end())
		  {
			  Throw(0, "Installation::ConvertSysPathToMacroPath(...) No macro defined: %s", macro);
		  }

		  cstr expansion = i->second.c_str();
		  if (strstr(fullPingPath, expansion) == nullptr)
		  {
			  Throw(0, "Installation::ConvertSysPathToMacroPath(...\"%s\", \"%s\") Path not prefixed by macro: %s", sysPath, macro, expansion);
		  }

		  SecureFormat(pingPath, pingPathCapacity, "%s/%s", macro, fullPingPath + i->second.size());
	  }

	  void ConvertSysPathToPingPath(cstr sysPath, char* pingPath, size_t pingPathCapacity) const override
	  {
		  if (pingPath == nullptr || sysPath == nullptr) Throw(0, "ConvertSysPathToPingPath: Null argument");

		  const fstring s = to_fstring(sysPath);
		  auto p = strstr(sysPath, contentDirectory.data);

		  int32 netLength = s.length - len;
		  if (netLength < 0 || p != sysPath)
		  {
			  Throw(0, "ConvertSysPathToPingPath: path did not begin with the content folder %s", contentDirectory.data);
		  }

		  if (netLength >= (int32)pingPathCapacity)
		  {
			  Throw(0, "ConvertSysPathToPingPath: Insufficient space in ping path buffer");
		  }

		  if (strstr(sysPath, "..") != nullptr)
		  {
			  Throw(0, "ConvertSysPathToPingPath: Illegal sequence in ping path: '..'");
		  }

		  SecureFormat(pingPath, pingPathCapacity, "!%s", sysPath + len);

		  OS::ToUnixPath(pingPath);
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

		  char pingRoot[IO::MAX_PATHLEN - 1];
		  int len = SecureFormat(pingRoot, sizeof(pingRoot), "%s", pingFolder);
		  OS::ToUnixPath(pingRoot);
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
   };

   class OSX : public IOSSupervisor
   {
      FilePath binDirectory;
      IEventCallback<SysUnstableArgs>* onUnstable;
   public:
      OSX() :
         onUnstable(nullptr)
      {
         uint32 lcapacity = _MAX_PATH;
         if (_NSGetExecutablePath(binDirectory, &lcapacity) != 0)
         {
            Throw(0, SEXTEXT("_NSGetExecutablePath failed"));
         }

         MakeContainerDirectory(binDirectory);
      }

      virtual ~OSX()
      {
      }

      void Free() override
      {
         delete this;
      }

      void Monitor(cstr absPath) override
      {

      }

      void UTF8ToUnicode(const char* s, wchar_t* unicode, size_t cbUtf8count, size_t unicodeCapacity) override
      {
         // OSX doesn't support wchar_t too well, so just do a bodge job
         size_t i = 0;
         for (; s[i] != 0; ++i)
         {
            if (i >= (unicodeCapacity - 1))
            {
               unicode[unicodeCapacity - 1] = 0;
               return;
            }
            else
            {
               unicode[i] = ((s[i] & 0x80) != 0) ? '?' : s[i];
            }
         }

         if (i >= (unicodeCapacity - 1))
         {
            unicode[unicodeCapacity - 1] = 0;
            return;
         }
         else
         {
            unicode[i] = 0;
         }
      }

      void EnumerateModifiedFiles(IEventCallback<FileModifiedArgs> &cb) override
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

      bool IsFileExistant(cstr absPath) const override
      {
         return OS::IsFileExistant(absPath);
      }

      void ConvertUnixPathToSysPath(cstr unixPath, rchar* sysPath, size_t bufferCapacity) const override
      {
         strcpy_s(sysPath, bufferCapacity, unixPath);
      }

      void LoadAbsolute(cstr absPath, IExpandingBuffer& buffer, int64 maxFileLength) const override
      {
         FileHandle hFile = fopen(absPath, "r");
         if (!hFile.IsValid()) Throw(errno, "OSX::LoadResource failed: Error opening file %s", absPath);

         fseek(hFile, 0, SEEK_END);
         long length = ftell(hFile);

         if ((int64)length > maxFileLength)
         {
            Throw(0, "OSX::LoadResource failed: File <%s> was too large at over %lld bytes", absPath, maxFileLength);
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
               Throw(0, "OSX::LoadResource: Error reading file <%s>. Failed to read chunk", absPath);
            }

            offset += (ptrdiff_t)chunk;
            bytesLeft -= (int64)chunk;
         }
      }

      void GetBinDirectoryAbsolute(rchar* directory, size_t capacityChars) const override
      {
         SecureFormat(directory, capacityChars, "%s", binDirectory.data);
      }

      size_t MaxPath() const override
      {
         return _MAX_PATH;
      }
   };
}

namespace Rococo
{
   IInstallationSupervisor* CreateInstallation(cstr contentIndicatorName, IOS& os)
   {
      return new Installation(contentIndicatorName, os);
   }
}

namespace Rococo
{
   IOSSupervisor* GetOS()
   {
      return new OSX();
   }

   namespace Windows
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
}