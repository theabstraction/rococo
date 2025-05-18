#include <sexy.types.h>
#include <sexy.strings.h>

#ifdef _WIN32
#include <malloc.h>
#endif

#include <float.h>
#include <stdio.h>
#include <memory.h>

#ifdef _WIN32
# include <cmath>
#endif

#include <rococo.parse.h>

using namespace Rococo;
using namespace Rococo::Parse;
using namespace Rococo::Strings;

namespace
{
	union DoubleBits
	{
		double DoubleValue;
		uint64 HexValue;
	};

	union FloatBits
	{
		float FloatValue;
		uint32 HexValue;
	};

	template<class T> T GetInfinity()
	{
		return T::NotImplemented();
	}

	template<class T> T GetNegativeInfinity()
	{
		return T::NotImplemented();
	}

	template<> double GetInfinity<double>()
	{
		DoubleBits X;
		X.HexValue = 0x7FFF000000000000;	
		return X.DoubleValue;
	}

	template<> double GetNegativeInfinity<double>()
	{
		DoubleBits X;
		X.HexValue = 0xFFF0000000000000;	
		return X.DoubleValue;
	}

	template<> float GetInfinity<float>()
	{
		FloatBits X;
		X.HexValue = 0x7F800000;	
		return X.FloatValue;
	}

	template<> float GetNegativeInfinity<float>()
	{
		FloatBits X;
		X.HexValue = 0xFF800000;	
		return X.FloatValue;
	}

	bool IsInfinity(double x)
	{
		DoubleBits X,Y;
		X.DoubleValue = GetInfinity<double>();
		Y.DoubleValue = x;

		return X.HexValue == Y.HexValue;
	}

	float IsInfinity(float x)
	{
		FloatBits X,Y;
		X.FloatValue = GetInfinity<float>();
		Y.FloatValue = x;

		return X.HexValue == Y.HexValue;
	}

	bool IsNegativeInfinity(double x)
	{
		DoubleBits X,Y;
		X.DoubleValue = GetNegativeInfinity<double>();
		Y.DoubleValue = x;

		return X.HexValue == Y.HexValue;
	}

	bool IsNegativeInfinity(float x)
	{
		FloatBits X,Y;
		X.FloatValue = GetNegativeInfinity<float>();
		Y.FloatValue = x;

		return X.HexValue == Y.HexValue;
	}

	bool TrySplitByChar(IN char splitChar1, IN char splitChar2, IN cstr s, OUT cstr& start, OUT cstr& end, REF char* tempBuffer, IN size_t charsInBuffer)
	{
		int len = StringLength(s);
		if (len >= (int) charsInBuffer)
		{
			return false;
		}

		memcpy_s(tempBuffer, sizeof(char) * charsInBuffer, s, sizeof(char) * len);
		tempBuffer[len] = 0;

		for(int i = 0; i < len; ++i)
		{
			if (tempBuffer[i] == splitChar1 || tempBuffer[i] == splitChar2)
			{
				tempBuffer[i] = 0;
				start = tempBuffer;
				end = tempBuffer + i + 1;
				return true;
			}
		}

		return false;
	}
}

namespace Rococo { namespace Compiler
{
	SEXYUTIL_API BITCOUNT GetBitCount(SexyVarType type)
	{
		switch(type)
		{
		case SexyVarType_Bool: return BITCOUNT_32;
		case SexyVarType_Int32: return BITCOUNT_32;
		case SexyVarType_Int64: return BITCOUNT_64;
		case SexyVarType_Float32: return BITCOUNT_32;
		case SexyVarType_Float64: return BITCOUNT_64;
		case SexyVarType_Derivative: return BITCOUNT_POINTER;
		case SexyVarType_Array: return BITCOUNT_POINTER;
		case SexyVarType_List: return BITCOUNT_POINTER;
		case SexyVarType_Map: return BITCOUNT_POINTER;
		case SexyVarType_Pointer: return BITCOUNT_POINTER;
		case SexyVarType_Closure: return BITCOUNT_128;
		default:
			return BITCOUNT_BAD;
		}
	}
}}

namespace Rococo { namespace Parse
{
	SEXYUTIL_API SexyVarType GetLiteralType(cstr candidate)
	{
		int32 iValue;
		if (TryParseDecimal(iValue, candidate) == PARSERESULT_GOOD)
		{
			return SexyVarType_Int32;
		}

		if (AreEqual(candidate, ("true")))
		{
			return SexyVarType_Bool;
		}

		if (AreEqual(candidate, ("false")))
		{
			return SexyVarType_Bool;
		}

		int64 i64Value;
		if (TryParseDecimal(i64Value, candidate) == PARSERESULT_GOOD)
		{
			return SexyVarType_Int64;
		}

		float fvalue;
		if (TryParseFloat(fvalue, candidate) == PARSERESULT_GOOD)
		{
			return SexyVarType_Float32;
		}

		double dvalue;
		PARSERESULT dresult = TryParseFloat(dvalue, candidate);
		if (dresult == PARSERESULT_OVERFLOW || dresult == PARSERESULT_UNDERFLOW || dresult == PARSERESULT_GOOD)
		{
			return SexyVarType_Float64;
		}
		
		if (Strings::Compare(candidate, "0x", 2) == 0)
		{
			if (TryParseHex(iValue, candidate + 2) == PARSERESULT_GOOD)
			{
				return SexyVarType_Int32;
			}

			if (TryParseHex(i64Value, candidate + 2) == PARSERESULT_GOOD)
			{
				return SexyVarType_Int64;
			}
		}

		return SexyVarType_Bad;
	}

	SEXYUTIL_API cstr VarTypeName(SexyVarType type)
	{
		switch (type)
		{
		case SexyVarType_Int32:	return ("Int32");
		case SexyVarType_Int64: return ("Int64");
		case SexyVarType_Float32: return ("Float32");
		case SexyVarType_Float64: return ("Float64");
		case SexyVarType_Bool: return ("Boolean32");
		case SexyVarType_Derivative: return ("Derivative");
		case SexyVarType_Pointer: return ("Pointer");
		default: return ("Unknown variable type");
		}
	}

#ifndef _WIN32
# define sscanf_s sscanf
#endif

	inline int ScanString(const char* decimalDigits, float32& value)
	{
		return sscanf_s(decimalDigits, "%f", &value);
	}

#ifdef char_IS_WIDE
	inline int ScanString(cstr decimalDigits, float32& value)
	{
		return swscanf_s(decimalDigits, L"%f", &value);
	}
#endif

	inline int ScanString(const char* decimalDigits, float64& value)
	{
		return sscanf_s(decimalDigits, "%lf", &value);
	}

#ifdef char_IS_WIDE
	inline int ScanString(cstr decimalDigits, float64& value)
	{
		return swscanf_s(decimalDigits, L"%lf", &value);
	}
#endif

	SEXYUTIL_API PARSERESULT TryParse(OUT VariantValue& value, SexyVarType type, IN cstr valueLiteral)
	{
		if (Strings::Compare(valueLiteral, "0x", 2) == 0)
		{
			switch (type)
			{
			case SexyVarType_Int32:
				return TryParseHex(OUT value.int32Value, IN valueLiteral + 2);
			case SexyVarType_Int64:
				return TryParseHex(OUT value.int64Value, IN valueLiteral + 2);
			case SexyVarType_Float32:
				return PARSERESULT_HEX_FOR_FLOAT;
			case SexyVarType_Float64:
				return PARSERESULT_HEX_FOR_FLOAT;
			default:
				return PARSERESULT_UNHANDLED_TYPE;
			}
		}

		switch(type)
		{
		case SexyVarType_Bool:
			return TryParseBoolean(OUT value.int32Value, IN valueLiteral);
		case SexyVarType_Int32:
			return TryParseDecimal(OUT value.int32Value, IN valueLiteral);
		case SexyVarType_Int64:
			return TryParseDecimal(OUT value.int64Value, IN valueLiteral);
		case SexyVarType_Float32:
			return TryParseFloat(OUT value.floatValue, IN valueLiteral);
		case SexyVarType_Float64:
			return TryParseFloat(OUT value.doubleValue, IN valueLiteral);
		default:
			return PARSERESULT_UNHANDLED_TYPE;
		}
	}
}} // Rococo::Compiler