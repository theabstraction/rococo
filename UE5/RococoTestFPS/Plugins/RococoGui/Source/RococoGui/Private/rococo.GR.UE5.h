#pragma once

#pragma once
#define ROCOCO_API __declspec(dllimport)
#define ROCOCO_UTIL_API __declspec(dllimport)
#define SEXYUTIL_API __declspec(dllimport)
#define SCRIPTEXPORT_API __declspec(dllimport)
#define ROCOCO_SEXML_API __declspec(dllimport)

#ifndef ROCOCO_GUI_RETAINED_API
# define ROCOCO_GUI_RETAINED_API _declspec(dllimport)
#endif

#ifndef ROCOCO_GREAT_SEX_API
# define  ROCOCO_GREAT_SEX_API _declspec(dllexport)
#endif

#define ROCOCO_USE_SAFE_V_FORMAT
