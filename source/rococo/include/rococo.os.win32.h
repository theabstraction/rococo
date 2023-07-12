#ifndef ROCOCO_WIN32_H
#define ROCOCO_WIN32_H

# include <rococo.types.h>

#ifdef _WIN32
# include <rococo.win32.target.win7.h>
# define WIN32_LEAN_AND_MEAN 
# define NOMINMAX
# include <windows.h>

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

   inline HWND ToHWND(WindowRef ref) { return (HWND) ref.pValue;  }
   inline WindowRef ToRef(HWND hWnd) { WindowRef ref; ref.pValue = hWnd; return ref; }
}

# endif
#endif
