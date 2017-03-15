#ifndef ROCOCO_WIN32_H
#define ROCOCO_WIN32_H

#define WIN32_LEAN_AND_MEAN 
#define NOMINMAX
#include <windows.h>

namespace Rococo
{
   class FileHandle
   {
      HANDLE hFile;
   public:
      FileHandle(HANDLE _hFile) : hFile(_hFile)
      {
      }

      operator HANDLE ()
      {
         return hFile;
      }

      ~FileHandle()
      {
         if (hFile != INVALID_HANDLE_VALUE) CloseHandle(hFile);
      }
   };
}

#endif
