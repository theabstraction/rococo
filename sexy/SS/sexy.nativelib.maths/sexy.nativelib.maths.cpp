// sexy.nativelib.maths.cpp : Defines the exported functions for the DLL application.
//

#include "sexy.nativelib.maths.stdafx.h"
#include "sexy.types.h"
#include "sexy.debug.types.h"
#include "sexy.compiler.public.h"
#include "sexy.script.h"
#include "sexy.native.sys.type.h"

#ifdef SEXCHAR_IS_WIDE
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

#include <rococo.win32.target.win7.h>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#undef min
#undef max

using namespace Sexy;
using namespace Sexy::Script;
using namespace Sexy::Compiler;
using namespace Sexy::SysType;
using namespace Sexy::Sex;

#include "sexy.vm.cpu.h"

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

#include "sys.maths.f32.inl"
#include "sys.maths.f64.inl"
#include "sys.maths.i32.inl"
#include "sys.maths.i64.inl"

#include "sexy.types.h"
#include "sexy.compiler.public.h"
#include "sexy.strings.h"

BOOLEAN WINAPI DllMain(HINSTANCE hDllHandle, DWORD nReason, LPVOID Reserved)
{
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

extern "C"
{
	__declspec(dllexport) INativeLib* CreateLib(Sexy::Script::IScriptSystem& ss)
	{
		class MathsNativeLib: public INativeLib
		{
		private:
			IScriptSystem& ss;

		public:
			MathsNativeLib(IScriptSystem& _ss): ss(_ss)
			{
			}

		private:
			virtual void AddNativeCalls()
			{
				Sys::Maths::F32::AddNativeCalls_SysMathsF32(ss);
				Sys::Maths::F64::AddNativeCalls_SysMathsF64(ss);
				Sys::Maths::I32::AddNativeCalls_SysMathsI32(ss);
				Sys::Maths::I64::AddNativeCalls_SysMathsI64(ss);
			}

			virtual void ClearResources()
			{
			}

			virtual void Release() 
			{
				delete this;
			}
		};
		return new MathsNativeLib(ss);
	}
}


