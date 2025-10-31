// Copyright (c)2025 Mark Anthony Taylor. Email: mark.anthony.taylor@gmail.com. All rights reserved.
#ifndef ROCOCO_WIN32_H
#define ROCOCO_WIN32_H

# include <rococo.types.h>

#ifdef _WIN32

namespace MSWindows
{
    typedef long long HANDLE_INTERNAL;

    struct HANDLE
    {
        HANDLE_INTERNAL internal;

        bool operator == (HANDLE other) const
        {
            return internal == other.internal;
        }

        bool operator != (HANDLE other) const
        {
            return internal != other.internal;
        }

        bool IsValidHandleValue() const
        {
            return internal != -1;
        }

        HANDLE operator = (const void* other)
        {
            internal = reinterpret_cast<HANDLE_INTERNAL>(other);
            return *this;
        }
    };

    struct HMODULE
    {
        HANDLE_INTERNAL ptrInternal;

        operator bool() const
        {
            return ptrInternal != 0;
        }
    };

    typedef HMODULE HINSTANCE;

    struct HWND
    {
        HANDLE_INTERNAL ptrInternal = 0;
    };

    typedef unsigned __int64 ULONG_PTR, * PULONG_PTR;
    typedef __int32 BOOL;
    typedef unsigned __int32 DWORD;
    typedef const void* LPCVOID;
    typedef void* LPVOID;
    typedef DWORD* LPDWORD;
    typedef void* PVOID;
    typedef const char* LPCSTR;
    typedef char* LPSTR;
    typedef wchar_t* LPWSTR;
    typedef const wchar_t* LPCWSTR;

#ifndef TRUE
    enum TruthValues
    {
        FALSE = 0,
        TRUE = 1
    };
#endif

    extern "C" __declspec(dllimport) BOOL IsDebuggerPresent();
    extern "C" __declspec(dllimport) void OutputDebugStringA(LPCSTR lpOutputString);
    extern "C" __declspec(dllimport) HMODULE LoadLibraryA(LPCSTR lpLibFileName);
    extern "C" __declspec(dllimport) HMODULE LoadLibraryW(LPCWSTR lpLibFileName);
    extern "C" __declspec(dllimport) HMODULE GetModuleHandleA(LPCSTR lpModuleName);
    extern "C" __declspec(dllimport) HMODULE GetModuleHandleW(LPCWSTR lpModuleName);
    extern "C" __declspec(dllimport) DWORD GetModuleFileNameA(HMODULE hModule, LPSTR lpFilename, DWORD nSize);
    extern "C" __declspec(dllimport) DWORD GetModuleFileNameW(HMODULE hModule, LPWSTR lpFilename, DWORD nSize);
    extern "C" __declspec(dllimport) DWORD GetCurrentDirectoryA(DWORD nBufferLength, LPSTR lpBuffer);
    extern "C" __declspec(dllimport) DWORD GetCurrentDirectoryW(DWORD nBufferLength, LPWSTR lpBuffer);

    extern "C" __declspec(dllimport) DWORD GetEnvironmentVariableA(LPCSTR lpName, LPSTR lpBuffer, DWORD nSize);
    extern "C" __declspec(dllimport) DWORD GetEnvironmentVariableW(LPCWSTR lpName, LPWSTR lpBuffer, DWORD nSize);

    extern "C" __declspec(dllimport) DWORD GetFileAttributesA(LPCSTR lpFileName);
    extern "C" __declspec(dllimport) DWORD GetFileAttributesW(LPCWSTR lpFileName);

    extern "C" __declspec(dllimport) BOOL SetEnvironmentVariableA(LPCSTR lpName, LPCSTR lpValue);
    extern "C" __declspec(dllimport) BOOL SetEnvironmentVariableW(LPCWSTR lpName, LPCWSTR lpValue);

    typedef int (*FARPROC)();

    extern "C" __declspec(dllimport) FARPROC GetProcAddress(HMODULE hModule, LPCSTR lpProcName);

    enum FileAttributes
    {
        INVALID_FILE_ATTRIBUTES = -1
    };

    enum EDLL
    {
        DLL_PROCESS_ATTACH = 1,
        DLL_THREAD_ATTACH = 2,
        DLL_THREAD_DETACH = 3,
        DLL_PROCESS_DETACH = 0
    };

    extern "C" __declspec(dllimport) BOOL DisableThreadLibraryCalls(HMODULE hLibModule);

    extern "C" __declspec(dllimport) DWORD GetTempPathA(DWORD nBufferLength, LPSTR lpBuffer);

    struct OVERLAPPED
    {
        ULONG_PTR Internal;
        ULONG_PTR InternalHigh;
        union UOffsetsAndPointers
        {
            struct UOffsets
            {
                DWORD Offset;
                DWORD OffsetHigh;
            } offsets;
            PVOID Pointer;
        } onp;

        HANDLE  hEvent;
    };

    typedef long LONG;
    typedef long long LONGLONG;

    union LARGE_INTEGER
    {
        struct
        {
            DWORD LowPart;
            LONG HighPart;
        } u;
        LONGLONG QuadPart;
    };

    typedef LARGE_INTEGER* PLARGE_INTEGER;

    static_assert(sizeof(LARGE_INTEGER) == sizeof(LONGLONG));

    static_assert(sizeof(OVERLAPPED) == 4 * sizeof(void*));

    static_assert(sizeof(void*) == sizeof(HANDLE));

    typedef OVERLAPPED* LPOVERLAPPED;

    typedef size_t SIZE_T;
    typedef __int8 BYTE;
    typedef __int16 WORD;

    struct SECURITY_ATTRIBUTES
    {
        DWORD nLength;
        LPVOID lpSecurityDescriptor;
        BOOL bInheritHandle;
    };

    static_assert(sizeof(SECURITY_ATTRIBUTES) == 3 * sizeof(void*));

    typedef struct _SECURITY_ATTRIBUTES* PSECURITY_ATTRIBUTES;

    typedef struct _SECURITY_ATTRIBUTES* LPSECURITY_ATTRIBUTES;

    extern "C" __declspec(dllimport) BOOL CloseHandle(HANDLE hFile);
    extern "C" __declspec(dllimport) BOOL ReadFile(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped);
    extern "C" __declspec(dllimport) BOOL WriteFile(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped);
    extern "C" __declspec(dllimport) DWORD GetLastError();
    extern "C" __declspec(dllimport) HANDLE CreateFileA(LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile);
    extern "C" __declspec(dllimport) HANDLE CreateFileW(LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile);

    extern "C" __declspec(dllimport) BOOL GetFileSizeEx(HANDLE hFile, PLARGE_INTEGER lpFileSize);
    extern "C" __declspec(dllimport) DWORD GetFileType(HANDLE hFile);

    extern "C" __declspec(dllimport) BOOL FreeLibrary(HMODULE hLibModule);

    extern "C" __declspec(dllimport) void Sleep(DWORD dwMilliseconds);

    struct PROCESS_HEAP_ENTRY
    {
        PROCESS_HEAP_ENTRY() {}

        PVOID lpData;
        DWORD cbData;
        BYTE cbOverhead;
        BYTE iRegionIndex;
        WORD wFlags;
        union Items
        {
            struct Block
            {
                HANDLE hMem;
                DWORD dwReserved[3];
            } block;
            struct Region
            {
                DWORD dwCommittedSize;
                DWORD dwUnCommittedSize;
                LPVOID lpFirstBlock;
                LPVOID lpLastBlock;
            } region;
        } items;
    };

    static_assert(sizeof(PROCESS_HEAP_ENTRY) == 40);

    typedef PROCESS_HEAP_ENTRY* LPPROCESS_HEAP_ENTRY;
    typedef PROCESS_HEAP_ENTRY* PPROCESS_HEAP_ENTRY;

    enum MSConstants
    {
        SYNCHRONIZE = 0x00100000ULL,
        STANDARD_RIGHTS_REQUIRED = 0x000F0000ULL,
        READ_CONTROL = 0x00020000ULL,
        STANDARD_RIGHTS_READ = READ_CONTROL,
        STANDARD_RIGHTS_WRITE = READ_CONTROL,
        STANDARD_RIGHTS_EXECUTE = READ_CONTROL,

        FILE_READ_DATA = 0x0001,    // file & pipe
        FILE_LIST_DIRECTORY = 0x0001,    // directory

        FILE_WRITE_DATA = 0x0002,    // file & pipe
        FILE_ADD_FILE = 0x0002,    // directory

        FILE_APPEND_DATA = 0x0004,    // file
        FILE_ADD_SUBDIRECTORY = 0x0004,    // directory
        FILE_CREATE_PIPE_INSTANCE = 0x0004,    // named pipe


        FILE_READ_EA = 0x0008,    // file & directory
        FILE_WRITE_EA = 0x0010,    // file & directory

        FILE_EXECUTE = 0x0020,    // file
        FILE_TRAVERSE = 0x0020,    // directory

        FILE_DELETE_CHILD = 0x0040,    // directory
        FILE_READ_ATTRIBUTES = 0x0080,    // all
        FILE_WRITE_ATTRIBUTES = 0x0100,    // all

        FILE_ALL_ACCESS = (STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | 0x1FF),

        FILE_GENERIC_READ = (STANDARD_RIGHTS_READ | FILE_READ_DATA | FILE_READ_ATTRIBUTES | FILE_READ_EA | SYNCHRONIZE),
        FILE_GENERIC_WRITE = (STANDARD_RIGHTS_WRITE | FILE_WRITE_DATA | FILE_WRITE_ATTRIBUTES | FILE_WRITE_EA | FILE_APPEND_DATA | SYNCHRONIZE),

        FILE_GENERIC_EXECUTE = (STANDARD_RIGHTS_EXECUTE | FILE_READ_ATTRIBUTES | FILE_EXECUTE | SYNCHRONIZE),

        FILE_SHARE_READ = 0x00000001ULL,
        FILE_SHARE_WRITE = 0x00000002ULL,
        FILE_SHARE_DELETE = 0x00000004ULL,
        FILE_ATTRIBUTE_READONLY = 0x00000001ULL,
        FILE_ATTRIBUTE_HIDDEN = 0x00000002ULL,
        FILE_ATTRIBUTE_SYSTEM = 0x00000004ULL,
        FILE_ATTRIBUTE_DIRECTORY = 0x00000010ULL,
        FILE_ATTRIBUTE_ARCHIVE = 0x00000020ULL,
        FILE_ATTRIBUTE_DEVICE = 0x00000040ULL,
        FILE_ATTRIBUTE_NORMAL = 0x00000080ULL,
        FILE_ATTRIBUTE_TEMPORARY = 0x00000100ULL,
        FILE_ATTRIBUTE_SPARSE_FILE = 0x00000200ULL,
        FILE_ATTRIBUTE_REPARSE_POINT = 0x00000400ULL,
        FILE_ATTRIBUTE_COMPRESSED = 0x00000800ULL,
        FILE_ATTRIBUTE_OFFLINE = 0x00001000ULL,
        FILE_ATTRIBUTE_NOT_CONTENT_INDEXED = 0x00002000ULL,
        FILE_ATTRIBUTE_ENCRYPTED = 0x00004000ULL,
        FILE_ATTRIBUTE_INTEGRITY_STREAM = 0x00008000ULL,
        FILE_ATTRIBUTE_VIRTUAL = 0x00010000ULL,
        FILE_ATTRIBUTE_NO_SCRUB_DATA = 0x00020000ULL,
        FILE_ATTRIBUTE_EA = 0x00040000ULL,
        FILE_ATTRIBUTE_PINNED = 0x00080000ULL,
        FILE_ATTRIBUTE_UNPINNED = 0x00100000ULL,
        FILE_ATTRIBUTE_RECALL_ON_OPEN = 0x00040000ULL,
        FILE_ATTRIBUTE_RECALL_ON_DATA_ACCESS = 0x00400000ULL,

        FILE_FLAG_WRITE_THROUGH = 0x80000000ULL,
        FILE_FLAG_OVERLAPPED = 0x40000000ULL,
        FILE_FLAG_NO_BUFFERING = 0x20000000ULL,
        FILE_FLAG_RANDOM_ACCESS = 0x10000000ULL,
        FILE_FLAG_SEQUENTIAL_SCAN = 0x08000000ULL,
        FILE_FLAG_DELETE_ON_CLOSE = 0x04000000ULL,
        FILE_FLAG_BACKUP_SEMANTICS = 0x02000000ULL,
        FILE_FLAG_POSIX_SEMANTICS = 0x01000000ULL,
        FILE_FLAG_SESSION_AWARE = 0x00800000ULL,
        FILE_FLAG_OPEN_REPARSE_POINT = 0x00200000ULL,
        FILE_FLAG_OPEN_NO_RECALL = 0x00100000ULL,
        FILE_FLAG_FIRST_PIPE_INSTANCE = 0x00080000ULL,

        GENERIC_READ = 0x80000000ULL,
        GENERIC_WRITE = 0x40000000ULL,

        CREATE_NEW = 1,
        CREATE_ALWAYS = 2,
        OPEN_EXISTING = 3,
        OPEN_ALWAYS = 4,
        TRUNCATE_EXISTING = 5
    };

    struct HDC
    {
        HANDLE_INTERNAL pInternal;
    };

    struct HICON
    {
        HANDLE_INTERNAL pInternal;
    };

    struct HMENU
    {
        HANDLE_INTERNAL pInternal;
    };

    extern "C" __declspec(dllimport) BOOL DestroyWindow(HWND hWnd);
    extern "C" __declspec(dllimport) BOOL ShowWindow(HWND hWnd, int nCmdShow);

    ROCOCO_API int GetWindowTitle(Rococo::Windows::IWindow& window, char* title, size_t capacity);
    ROCOCO_API Rococo::Windows::IWindow& NullParent();
    ROCOCO_API int ShowMessageBox(Rococo::Windows::IWindow& window, const char* text, const char* caption, Rococo::uint32 type);
}

namespace Rococo
{
   class FileHandle
   {
       MSWindows::HANDLE hFile;
   public:
      FileHandle(MSWindows::HANDLE _hFile) : hFile(_hFile)
      {
      }

      operator MSWindows::HANDLE ()
      {
         return hFile;
      }

      ~FileHandle()
      {
         if (hFile.IsValidHandleValue()) MSWindows::CloseHandle(hFile);
      }
   };

   inline MSWindows::HWND ToHWND(Rococo::WindowRef ref) { return MSWindows::HWND{ reinterpret_cast<MSWindows::HANDLE_INTERNAL>(ref.pValue) }; }
   inline WindowRef ToRef(MSWindows::HWND hWnd) { WindowRef ref; ref.pValue = reinterpret_cast<void*>(hWnd.ptrInternal); return ref; }
}

# endif
#endif
