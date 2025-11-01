// Copyright (c)2025 Mark Anthony Taylor. Email: mark.anthony.taylor@gmail.com. All rights reserved.
#ifndef ROCOCO_WIN32_H
#define ROCOCO_WIN32_H

#ifdef _WIN32

# include <rococo.types.h>

# ifdef _WINDOWS_
#  error _WINDOWS_ already defined Windows.h already included apparently
# endif

#define ROCOCO_OS_WIN32_API extern "C" __declspec(dllimport)

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

        static HMODULE Null() { return { 0 }; }
    };

    typedef HMODULE HINSTANCE;

    struct HWND
    {
        HANDLE_INTERNAL ptrInternal = 0;

        static HWND Null() { return  { 0 }; }
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

    ROCOCO_OS_WIN32_API BOOL IsDebuggerPresent();
    ROCOCO_OS_WIN32_API void OutputDebugStringA(LPCSTR lpOutputString);
    ROCOCO_OS_WIN32_API HMODULE LoadLibraryA(LPCSTR lpLibFileName);
    ROCOCO_OS_WIN32_API HMODULE LoadLibraryW(LPCWSTR lpLibFileName);
    ROCOCO_OS_WIN32_API HMODULE GetModuleHandleA(LPCSTR lpModuleName);
    ROCOCO_OS_WIN32_API HMODULE GetModuleHandleW(LPCWSTR lpModuleName);
    ROCOCO_OS_WIN32_API DWORD GetModuleFileNameA(HMODULE hModule, LPSTR lpFilename, DWORD nSize);
    ROCOCO_OS_WIN32_API DWORD GetModuleFileNameW(HMODULE hModule, LPWSTR lpFilename, DWORD nSize);
    ROCOCO_OS_WIN32_API DWORD GetCurrentDirectoryA(DWORD nBufferLength, LPSTR lpBuffer);
    ROCOCO_OS_WIN32_API DWORD GetCurrentDirectoryW(DWORD nBufferLength, LPWSTR lpBuffer);

    ROCOCO_OS_WIN32_API DWORD GetEnvironmentVariableA(LPCSTR lpName, LPSTR lpBuffer, DWORD nSize);
    ROCOCO_OS_WIN32_API DWORD GetEnvironmentVariableW(LPCWSTR lpName, LPWSTR lpBuffer, DWORD nSize);

    ROCOCO_OS_WIN32_API DWORD GetFileAttributesA(LPCSTR lpFileName);
    ROCOCO_OS_WIN32_API DWORD GetFileAttributesW(LPCWSTR lpFileName);

    ROCOCO_OS_WIN32_API BOOL SetEnvironmentVariableA(LPCSTR lpName, LPCSTR lpValue);
    ROCOCO_OS_WIN32_API BOOL SetEnvironmentVariableW(LPCWSTR lpName, LPCWSTR lpValue);

    typedef int (*FARPROC)();

    ROCOCO_OS_WIN32_API FARPROC GetProcAddress(HMODULE hModule, LPCSTR lpProcName);

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

    ROCOCO_OS_WIN32_API BOOL DisableThreadLibraryCalls(HMODULE hLibModule);

    ROCOCO_OS_WIN32_API DWORD GetTempPathA(DWORD nBufferLength, LPSTR lpBuffer);

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

    ROCOCO_OS_WIN32_API BOOL CloseHandle(HANDLE hFile);
    ROCOCO_OS_WIN32_API BOOL ReadFile(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped);
    ROCOCO_OS_WIN32_API BOOL WriteFile(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped);
    ROCOCO_OS_WIN32_API DWORD GetLastError();
    ROCOCO_OS_WIN32_API HANDLE CreateFileA(LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile);
    ROCOCO_OS_WIN32_API HANDLE CreateFileW(LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile);

    ROCOCO_OS_WIN32_API BOOL GetFileSizeEx(HANDLE hFile, PLARGE_INTEGER lpFileSize);
    ROCOCO_OS_WIN32_API DWORD GetFileType(HANDLE hFile);

    ROCOCO_OS_WIN32_API BOOL FreeLibrary(HMODULE hLibModule);

    ROCOCO_OS_WIN32_API void Sleep(DWORD dwMilliseconds);

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

    ROCOCO_OS_WIN32_API BOOL DestroyWindow(HWND hWnd);
    ROCOCO_OS_WIN32_API BOOL ShowWindow(HWND hWnd, int nCmdShow);

    ROCOCO_API int GetWindowTitle(Rococo::Windows::IWindow& window, char* title, size_t capacity);
    ROCOCO_API Rococo::Windows::IWindow& NullParent();
    ROCOCO_API int ShowMessageBox(Rococo::Windows::IWindow& window, const char* text, const char* caption, Rococo::uint32 type);

    typedef long LSTATUS;

    typedef HANDLE HKEY;
    typedef HKEY* PHKEY;

    typedef DWORD ACCESS_MASK;
    typedef ACCESS_MASK* PACCESS_MASK;

    typedef ACCESS_MASK REGSAM;

    typedef BYTE* LPBYTE;

    ROCOCO_OS_WIN32_API LSTATUS RegOpenKeyA(HKEY hKey, LPCSTR lpSubKey, PHKEY phkResult);
    ROCOCO_OS_WIN32_API LSTATUS RegOpenKeyW(HKEY hKey, LPCWSTR lpSubKey, PHKEY phkResult);
    ROCOCO_OS_WIN32_API LSTATUS RegOpenKeyExA(HKEY hKey, LPCSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, PHKEY phkResult);
    ROCOCO_OS_WIN32_API LSTATUS RegOpenKeyExW(HKEY hKey, LPCWSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, PHKEY phkResult);


# define HKEY_CLASSES_ROOT        ( HKEY { 0x80000000ULL } )
# define HKEY_CURRENT_USER        ( HKEY { 0x80000001ULL } )
# define HKEY_LOCAL_MACHINE       ( HKEY { 0x80000002ULL } )
# define HKEY_USERS               ( HKEY { 0x80000003ULL } )
# define HKEY_PERFORMANCE_DATA    ( HKEY { 0x80000004ULL } )
# define HKEY_PERFORMANCE_TEXT    ( HKEY { 0x80000050ULL } )
# define HKEY_PERFORMANCE_NLSTEXT ( HKEY { 0x80000060ULL } )

    enum EShowWindow
    {
        SW_HIDE = 0,
        SW_SHOWNORMAL = 1,
        SW_NORMAL = 1,
        SW_SHOWMINIMIZED = 2,
        SW_SHOWMAXIMIZED = 3,
        SW_MAXIMIZE = 3,
        SW_SHOWNOACTIVATE = 4,
        SW_SHOW = 5,
        SW_MINIMIZE = 6,
        SW_SHOWMINNOACTIVE = 7,
        SW_SHOWNA = 8,
        SW_RESTORE = 9,
        SW_SHOWDEFAULT = 10,
        SW_FORCEMINIMIZE = 11,
        SW_MAX = 11,
    };

    ROCOCO_OS_WIN32_API LSTATUS RegQueryValueExA(HKEY hKey, LPCSTR lpValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData);
    ROCOCO_OS_WIN32_API LSTATUS RegQueryValueExW(HKEY hKey, LPCWSTR lpValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData);
    ROCOCO_OS_WIN32_API LSTATUS RegCloseKey(HKEY hKey);

    enum ERegTypes
    {
        REG_NONE = 0UL,
        REG_SZ = 1UL,
        REG_EXPAND_SZ = 2UL,
        REG_BINARY = 3ul,
        REG_DWORD = 4ul,
        REG_DWORD_LITTLE_ENDIAN = 4ul,
        REG_DWORD_BIG_ENDIAN = 5ul,
        REG_LINK = 6ul,
        REG_MULTI_SZ = 7ul,
        REG_RESOURCE_LIST = 8ul,
        REG_FULL_RESOURCE_DESCRIPTOR = 9ul,
        REG_RESOURCE_REQUIREMENTS_LIST = 10ul,
        REG_QWORD = 11ul,
        REG_QWORD_LITTLE_ENDIAN = 11ul
    };

    ROCOCO_OS_WIN32_API HWND GetConsoleWindow();

    typedef unsigned __int32 UINT;

    ROCOCO_OS_WIN32_API int MessageBoxA(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType);
    ROCOCO_OS_WIN32_API int MessageBoxW(HWND hWnd, LPCWSTR lpText, LPCWSTR lpCaption, UINT uType);

    enum EMessageBoxCode
    {
        MB_OK = 0x00000000L,
        MB_OKCANCEL = 0x00000001L,
        MB_ABORTRETRYIGNORE = 0x00000002L,
        MB_YESNOCANCEL = 0x00000003L,
        MB_YESNO = 0x00000004L,
        MB_RETRYCANCEL = 0x00000005L,
        MB_CANCELTRYCONTINUE = 0x00000006L,
        MB_ICONHAND = 0x00000010L,
        MB_ICONQUESTION = 0x00000020L,
        MB_ICONEXCLAMATION = 0x00000030L,
        MB_ICONASTERISK = 0x00000040L,
        MB_USERICON = 0x00000080L,
        MB_ICONWARNING = MB_ICONEXCLAMATION,
        MB_ICONERROR = MB_ICONHAND,
        MB_ICONINFORMATION = MB_ICONASTERISK,
        MB_ICONSTOP = MB_ICONHAND,
    };
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

namespace Rococo::Windows
{
    ROCOCO_INTERFACE IWindow
    {
        virtual operator MSWindows::HWND() const = 0;
    };
}

# endif
#endif
