#ifndef Rococo_Strings_H
#define Rococo_Strings_H

#include <wchar.h>

#define SecureFormat swprintf_s // Needs include <wchar.h>. If the output buffer is exhausted it will throw an exception
#define SafeFormat _snwprintf_s // Needs include <wchar.h>. With _TRUNCATE in the MaxCount position, it will truncate buffer overruns, rather than throw
#define SafeVFormat _vsnwprintf_s // Needs include <wchar.h>. With _TRUNCATE in the MaxCount position, it will truncate buffer overruns, rather than throw
#define SecureCopy wcscpy_s // Needs include <wchar.h>. If the output buffer is exhausted it will throw an exception
#define SafeCopy wcsncpy_s // Needs include <wchar.h>. With _TRUNCATE in the MaxCount position, it will truncate buffer overruns, rather than throw
#define SecureCat wcscat_s // Needs include <wchar.h>.  If the output buffer is exhausted it will throw an exception
#define SafeCat wcsncat_s // Needs include <wchar.h>.  With _TRUNCATE in the MaxCount position, it will truncate buffer overruns, rather than throw

#endif  Rococo_Strings_H