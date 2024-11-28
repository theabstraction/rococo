// sexy.nativelib.maths.cpp : Defines the exported functions for the DLL application.
//

#include "sexy.nativelib.maths.stdafx.h"
#include "sexy.types.h"
#include "sexy.debug.types.h"
#include "sexy.compiler.public.h"
#include "sexy.vm.h"
#include "sexy.vm.cpu.h"
#include "sexy.script.h"
#include "sexy.native.sys.type.h"
#include "../STC/stccore/sexy.compiler.h"
#include "sexy.strings.h"

#ifdef char_IS_WIDE
# ifndef UNICODE
#  define UNICODE
# endif
# ifndef _UNICODE
#  define _UNICODE
# endif
#endif

// Including SDKDDKVer.h defines the highest available Windows platform.

// If you wish to build your application for a previous Windows platform, include WinSDKVer.h and
// set the _WIN32_WINNT macro to the platform you wish to support before including SDKDDKVer.h.

#include "rococo.os.win32.h"

using namespace Rococo;
using namespace Rococo::Script;
using namespace Rococo::Compiler;
using namespace Rococo::SysType;
using namespace Rococo::Sex;

#include <cmath>
#include <limits>

template<typename T> T MaxOf(T a, T b)
{
	return a > b ? a : b;
}

template<typename T> T MinOf(T a, T b)
{
	return a < b ? a : b;
}

union FloatInt
{
	float fValue;
	int32 iValue;
};

#ifndef _WIN32
int32 abs(int32 x)
{
	return  x < 0 ? -x : x;
}

int64 llabs(int64 x)
{
	return  x < 0 ? -x : x;
}
#endif

boolean32 IsQuietNan(float f)
{
	FloatInt v;
	v.fValue = f;

	return ((v.iValue & 0x7FC00000) == 0x7FC00000) ? 1 : 0;
}

boolean32 IsSignalNan(float f)
{
	FloatInt v;
	v.fValue = f;

	return ((v.iValue & 0x7FC00000) == 0x7F800000) && ((v.iValue & 0x003FFFFF) != 0) ? 1 : 0;
}

boolean32 IsNan(float f)
{
	FloatInt v;
	v.fValue = f;

	return ((v.iValue & 0x7F800000) == 0x7F800000) && ((v.iValue & 0x003FFFFF) != 0) ? 1 : 0;
}

float QuietNanF32()
{
	FloatInt v;
	v.iValue = 0x7FC00000;
	return v.fValue;
}

union DoubleInt
{
	double fValue;
	int64 iValue;
};

boolean32 IsQuietNan(double f)
{
	DoubleInt v; // s111 1111 1111 axxx xxxx xxxx xxxx xxxx xxxx xxxx xxxx xxxx xxxx xxxx xxxx xxxx
	v.fValue = f;

	return ((v.iValue & 0x7FF8000000000000) == 0x7FF8000000000000) ? 1 : 0;
}

boolean32 IsSignalNan(double f)
{
	DoubleInt v;
	v.fValue = f;

	return ((v.iValue & 0x7FF800000000000) == 0x7FF000000000000) && ((v.iValue & 0x7FFFFFFFFFFF) != 0) ? 1 : 0;
}

boolean32 IsNan(double f)
{
	DoubleInt v;
	v.fValue = f;

	return ((v.iValue & 0x7FFF000000000000) == 0x7FFF000000000000) && ((v.iValue & 0x7FFFFFFFFFFF) != 0) ? 1 : 0;
}

double QuietNanF64()
{
	DoubleInt v;
	v.iValue = 0x7FF800000000000;
	return v.fValue;
}

template<class T>
boolean32 IsInfinity(T f)
{
	return std::isinf(f) ? 1 : 0;
}

template<class T>
boolean32 IsNormal(T f)
{
	return std::isnormal(f) ? 1 : 0;
}

template<class T>
boolean32 IsFinite(T f)
{
	return std::isfinite(f) ? 1 : 0;
}

float MinF32Value()
{
	return std::numeric_limits<float>::min();
}

float MaxF32Value()
{
	return std::numeric_limits<float>::max();
}

double MinF64Value()
{
	return std::numeric_limits<double>::min();
}

double MaxF64Value()
{
	return std::numeric_limits<double>::max();
}

int32 MinI32Value()
{
	return std::numeric_limits<int32>::min();
}

int32 MaxI32Value()
{
	return std::numeric_limits<int32>::max();
}

int64 MinI64Value()
{
	return std::numeric_limits<int64>::min();
}

int64 MaxI64Value()
{
	return std::numeric_limits<int64>::max();
}

template<class T>
T Mod(T numerator, T denominator)
{
	return numerator % denominator;
}

template<class T>
T LeftShift(T x, T bitCount)
{
	return x << bitCount;
}

template<class T>
T RightShift(T x, T bitCount)
{
	return x >> bitCount;
}

template<class T>
float ToFloat32(T t)
{
	return (float)t;
}

template<class T>
double ToFloat64(T t)
{
	return (double)t;
}

template<class T>
int32 ToInt32(T t)
{
	return (int32)t;
}

template<class T>
int64 ToInt64(T t)
{
	return (int64)t;
}

void SeedRandom(int seedValue)
{
	srand(seedValue);
}

#include "sys.maths.f32.inl"
#include "sys.maths.f64.inl"
#include "sys.maths.i32.inl"
#include "sys.maths.i64.inl"

#include "sexy.types.h"
#include "sexy.compiler.public.h"
#include "sexy.strings.h"

#ifdef _WIN32

BOOLEAN WINAPI DllMain(HINSTANCE hDllHandle, DWORD nReason, LPVOID reserved)
{
	UNUSED(reserved);

	BOOLEAN bSuccess = TRUE;

	switch (nReason)
	{
	case DLL_PROCESS_ATTACH:
		DisableThreadLibraryCalls(hDllHandle);
		break;
	case DLL_PROCESS_DETACH:
		break;
	}

	return bSuccess;
}

# define DLLEXPORT __declspec(dllexport)
#else
# define DLLEXPORT
#endif

extern "C"
{
	DLLEXPORT INativeLib* CreateLib(Rococo::Script::IScriptSystem& ss)
	{
		class MathsNativeLib : public INativeLib
		{
		private:
			IScriptSystem& ss;

		public:
			MathsNativeLib(IScriptSystem& _ss) : ss(_ss)
			{
			}

		private:
			void AddNativeCalls() override
			{
				Sys::Maths::F32::AddNativeCalls_SysMathsF32(ss);
				Sys::Maths::F64::AddNativeCalls_SysMathsF64(ss);
				Sys::Maths::I32::AddNativeCalls_SysMathsI32(ss);
				Sys::Maths::I64::AddNativeCalls_SysMathsI64(ss);
			}

			void ClearResources() override
			{

			}

			void Release() override
			{
				delete this;
			}
		};
		return new MathsNativeLib(ss);
	}
}


