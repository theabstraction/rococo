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
#include <rococo.strings.h>
#include <errno.h>
#include <mach-o/dyld.h>

namespace
{
   int breakFlags = 0;
}

namespace Rococo
{
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

   void Throw(int32 errorCode, cstr format, ...)
   {
      va_list args;
      va_start(args, format);

      struct : public IException
      {
         rchar msg[256];
         int32 errorCode;

         virtual cstr Message() const
         {
            return msg;
         }

         virtual int32 ErrorCode() const
         {
            return errorCode;
         }
      } ex;

      _vsnprintf_s(ex.msg, 256, _TRUNCATE, format, args);

      ex.errorCode = errorCode;

      OS::TripDebugger();

      throw ex;
   }
}

namespace Rococo
{
   bool FileModifiedArgs::Matches(cstr resource)
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

   void FileModifiedArgs::GetPingPath(rchar* path, size_t capacity)
   {
      SafeFormat(path, capacity, _TRUNCATE, "!%s", resourceName);
   }
}

namespace Rococo
{
   namespace OS
   {
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
         __builtin_trap();;
      }

      void SetBreakPoints(int flags)
      {
         breakFlags = flags;
      }

#ifdef BREAK_ON_THROW
      void BreakOnThrow(BreakFlag flag)
      {
         if ((breakFlags & flag) != 0 && IsDebuging())
         {
            TripDebugger();
         }
      }
#else
      void BreakOnThrow(BreakFlag flag) {}
#endif

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

      void LoadAsciiTextFile(SEXCHAR* data, size_t capacity, const SEXCHAR* filename)
      {
         FILE* f = fopen(filename, "rb");
         if (f == nullptr)
         {
            Throw(0, SEXTEXT("Cannot open file %s"), filename);
         }

         size_t startIndex = 0;
         while (startIndex < capacity)
         {
            size_t bytesRead = fread(data + startIndex, 1, capacity - startIndex, f);

            if (bytesRead == 0)
            {
               // graceful completion
               break;
            }
            startIndex += bytesRead;
         }

         if (startIndex >= capacity)
         {
            fclose(f);
            Throw(0, "File too large: ", filename);
         }

         fclose(f);

         data[startIndex] = 0;
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
            SecureFormat(path.data, "%s", contentIndicatorName);
            MakeContainerDirectory(path);
            return;
         }
      }

      size_t len = rlen(path);

      while (len > 0)
      {
         FilePath indicator;
         SecureFormat(indicator.data, "%s%s", path.data, contentIndicatorName);
         if (os.IsFileExistant(indicator))
         {
            SecureCat(path.data, "content/");
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

   public:
      Installation(cstr contentIndicatorName, IOS& _os) : os(_os)
      {
         GetContentDirectory(contentIndicatorName, contentDirectory, os);
      }

      virtual void Free()
      {
         delete this;
      }

      virtual IOS& OS()
      {
         return os;
      }

      virtual cstr Content() const
      {
         return contentDirectory.data;
      }

      virtual void LoadResource(cstr resourcePath, IExpandingBuffer& buffer, int64 maxFileLength)
      {
         if (resourcePath == nullptr || rlen(resourcePath) < 2) Throw(0, "Win32OS::LoadResource failed: <resourcePath> was blank");
         if (resourcePath[0] != '!') Throw(0, "Win32OS::LoadResource failed: <%s> did not begin with ping '!' character", resourcePath);

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
         SecureFormat(absPath.data, "%s%s", contentDirectory.data, sysPath.data);

         os.LoadAbsolute(absPath, buffer, maxFileLength);
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

      virtual void Free()
      {
         delete this;
      }

      virtual void UTF8ToUnicode(const char* s, wchar_t* unicode, size_t cbUtf8count, size_t unicodeCapacity)
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

      virtual void EnumerateModifiedFiles(IEventCallback<FileModifiedArgs> &cb)
      {
      }

      virtual void FireUnstable()
      {
         SysUnstableArgs unused;
         if (onUnstable) onUnstable->OnEvent(unused);
      }

      virtual void SetUnstableHandler(IEventCallback<SysUnstableArgs>* cb)
      {
         onUnstable = cb;
      }

      virtual bool IsFileExistant(cstr absPath) const
      {
         return OS::IsFileExistant(absPath);
      }

      virtual void ConvertUnixPathToSysPath(cstr unixPath, rchar* sysPath, size_t bufferCapacity) const
      {
         strcpy_s(sysPath, bufferCapacity, unixPath);
      }

      virtual void LoadAbsolute(cstr absPath, IExpandingBuffer& buffer, int64 maxFileLength) const
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
               Throw(0, "Win32OS::LoadResource: Error reading file <%s>. Failed to read chunk", absPath);
            }

            offset += (ptrdiff_t)chunk;
            bytesLeft -= (int64)chunk;
         }
      }

      virtual void GetBinDirectoryAbsolute(rchar* directory, size_t capacityChars) const
      {
         SecureFormat(directory, capacityChars, "%s", binDirectory.data);
      }

      virtual size_t MaxPath() const
      {
         return _MAX_PATH;
      }
   };
}

