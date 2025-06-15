#pragma once

#ifndef ROCOCO_UE5_EXPORT
# ifdef ROCOCO_BUILD_IS_MONOLITHIC
#  define ROCOCO_UE5_EXPORT
#  define ROCOCO_UE5_IMPORT
# else
#  define ROCOCO_UE5_EXPORT DLLEXPORT
#  define ROCOCO_UE5_IMPORT DLLIMPORT
# endif
#endif

#define __ROCOCO_WIDECHAR__ WIDECHAR

#ifdef PLATFORM_ANDROID
# define ROCOCO_WIDECHAR_IS_CHAR_16_T
#endif

#ifdef _WIN32
# define ROCOCO_WIDECHAR_IS_WCHAR_T
#endif

#define ROCOCO_JPEG_API __declspec(dllexport)
#define ROCOCO_TIFF_API ROCOCO_UE5_IMPORT