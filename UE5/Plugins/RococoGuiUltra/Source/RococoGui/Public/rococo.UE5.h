// Copyright (c) 2025 Mark Anthony Taylor. All rights reserved. Email: mark.anthony.taylor@gmail.com

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

#define _ROCOCO_WIDECHAR_ WIDECHAR

#ifdef PLATFORM_ANDROID
# define ROCOCO_WIDECHAR_IS_CHAR_16_T
#endif

#ifdef _WIN32
# define ROCOCO_WIDECHAR_IS_WCHAR_T
#endif

#define ROCOCO_BPUTIL_API ROCOCO_UE5_EXPORT

#ifndef ROCOCO_API
# define ROCOCO_API ROCOCO_UE5_EXPORT
#endif
#define ROCOCO_WINDOWS_API
#define ROCOCO_UTIL_API
#define SEXYUTIL_API
#define SCRIPTEXPORT_API
#define ROCOCO_SEXML_API
#define ROCOCO_GUI_RETAINED_API
#define ROCOCO_GREAT_SEX_API
#define SEXYUTIL_API
#define ROCOCO_JPEG_API
#define ROCOCO_TIFF_API
#define ROCOCO_USE_SAFE_V_FORMAT
#define ROCOCO_GUI_API ROCOCO_UE5_EXPORT