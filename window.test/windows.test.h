#ifndef Rococo_WINDOWS_TEST_H
#define Rococo_WINDOWS_TEST_H

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#include <Rococo.target.h>

#define NOMINMAX
#include <Windows.h>

#include <Rococo.api.h>
#include <Rococo.window.h>

#define ROCOCO_USE_SAFE_V_FORMAT
#include <rococo.strings.h>

#endif