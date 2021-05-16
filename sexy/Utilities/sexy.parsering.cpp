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

namespace
{
	using namespace Rococo;
	using namespace Rococo::Parse;

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

	// Attempt to parse a number in the format [+-]*(0-9).*(0.9)
	template<typename T> PARSERESULT _TryParseDecimalSequence(OUT T& y, IN cstr s)
	{
		T sign;
		int startPos;

		if (s[0] == '+')
		{
			startPos = 1;
			sign = 1.0;
		}
		else if (s[0] == '-')
		{
			startPos = 1;
			sign = -1.0;
		}
		else
		{
			startPos = 0;
			sign = 1.0f;
		}

		if (s[startPos] == 0)
		{
			return PARSERESULT_BAD_DECIMAL_DIGIT;
		}

		int dotPos = StringLength(s);

		for(int i = startPos; s[i] != 0; i++)
		{
			if (s[i] == '.')
			{
				dotPos = i;
				break;
			}
		}

		T base = 1.0;

		y = 0;

		for(int j = dotPos-1; j >= startPos; j--)
		{
			char c = s[j];
			if (c < '0' || c > '9')
			{
				return PARSERESULT_BAD_DECIMAL_DIGIT;
			}
		}

		if (s[dotPos] != 0)
		{
			for(int j = dotPos+1; s[j] != 0; j++)
			{
				char c = s[j];
				if (c < '0' || c > '9')
				{
					return PARSERESULT_BAD_DECIMAL_DIGIT;
				}
			}
		}

		bool wasValued = false;

		for(int j = dotPos-1; j >= startPos; j--)
		{
			char c = s[j];
			T unit = (T) (c - '0');
			T value = unit * base;
			base *= 10.0;

			y += value;

			if (IsInfinity(y))
			{
				if (sign < 0) { y = GetNegativeInfinity<T>(); }												
				return PARSERESULT_OVERFLOW;
			}
		}

		if (y > 0) wasValued = true;

		base = 1.0;

		if (s[dotPos] != 0)
		{
			for(int j = dotPos+1; s[j] != 0; j++)
			{
				base *= (T) 0.1;

				char c = s[j];

				T unit = (T) (c - '0');
				T value = unit * base;

				y += value;

				if (unit > 0) wasValued = true;
			}
		}

		if (IsInfinity(y))
		{
			if (sign < 0) { y = GetNegativeInfinity<T>(); }												
			return PARSERESULT_OVERFLOW;
		}

		if (y == 0 && wasValued == true)
		{
			return PARSERESULT_UNDERFLOW;
		}

		y *= sign;
		return PARSERESULT_GOOD;
	}

	template<typename T> PARSERESULT _TryParseExponentForm(OUT T& y, cstr s)
	{
		enum {MAX_CHARS = 64};
		char temp[MAX_CHARS];
		cstr mantissa, exponent;
		if (!TrySplitByChar('e', 'E', s, mantissa, exponent, temp, MAX_CHARS))
		{
			return PARSERESULT_UNHANDLED_TYPE;
		}

		int32 exponentValue;
		T f = 1.0;
		PARSERESULT re = TryParseDecimal(OUT exponentValue, IN exponent);
		if (re == PARSERESULT_GOOD)
		{
			T multiplier = (T)(exponentValue < 0 ? 0.1 : 10.0);
			int n = (exponentValue > 0) ? exponentValue : -exponentValue;
			while (n) 
			{
				if (n & 1) 
				{
					f *= multiplier;
				}
				n >>= 1;
				multiplier *= multiplier;
			}

			if (f == 0)
			{
				y = 0; 
				return PARSERESULT_UNDERFLOW;
			}
		}

		T mantissaValue;
		PARSERESULT rm = _TryParseDecimalSequence(OUT mantissaValue, IN mantissa);
		if (rm == PARSERESULT_BAD_DECIMAL_DIGIT)
		{
			return rm;
		}

		if (rm == PARSERESULT_OVERFLOW)
		{
			y = mantissaValue;
			return PARSERESULT_OVERFLOW;
		}

		if (IsInfinity(f))
		{
			y = (mantissaValue >= 0) ? GetInfinity<T>() : GetNegativeInfinity<T>();
			return PARSERESULT_OVERFLOW;
		}

		y = mantissaValue * f;

		if (y == 0 && f != 0)
		{
			return PARSERESULT_UNDERFLOW;
		}
		else if (IsInfinity(y))
		{
			return PARSERESULT_OVERFLOW;
		}
		else if (IsNegativeInfinity(y))
		{
			return PARSERESULT_OVERFLOW;
		}
		else
		{
			return PARSERESULT_GOOD;
		}
	}
}

namespace Rococo { namespace Compiler
{
	BITCOUNT GetBitCount(VARTYPE type)
	{
		switch(type)
		{
		case VARTYPE_Bool: return BITCOUNT_32;
		case VARTYPE_Int32: return BITCOUNT_32;
		case VARTYPE_Int64: return BITCOUNT_64;
		case VARTYPE_Float32: return BITCOUNT_32;
		case VARTYPE_Float64: return BITCOUNT_64;
		case VARTYPE_Derivative: return BITCOUNT_POINTER;
		case VARTYPE_Array: return BITCOUNT_POINTER;
		case VARTYPE_List: return BITCOUNT_POINTER;
		case VARTYPE_Map: return BITCOUNT_POINTER;
		case VARTYPE_Pointer: return BITCOUNT_POINTER;
		case VARTYPE_Closure: return BITCOUNT_128;
		default:
			return BITCOUNT_BAD;
		}
	}
}}

namespace Rococo { namespace Parse
{
	VARTYPE GetLiteralType(cstr candidate)
	{
		int32 iValue;
		if (TryParseDecimal(iValue, candidate) == PARSERESULT_GOOD)
		{
			return VARTYPE_Int32;
		}

		if (AreEqual(candidate, ("true")))
		{
			return VARTYPE_Bool;
		}

		if (AreEqual(candidate, ("false")))
		{
			return VARTYPE_Bool;
		}

		int64 i64Value;
		if (TryParseDecimal(i64Value, candidate) == PARSERESULT_GOOD)
		{
			return VARTYPE_Int64;
		}

		float fvalue;
		if (TryParseFloat(fvalue, candidate) == PARSERESULT_GOOD)
		{
			return VARTYPE_Float32;
		}

		double dvalue;
		PARSERESULT dresult = TryParseFloat(dvalue, candidate);
		if (dresult == PARSERESULT_OVERFLOW || dresult == PARSERESULT_UNDERFLOW || dresult == PARSERESULT_GOOD)
		{
			return VARTYPE_Float64;
		}
		
		if (Compare(candidate, ("0x"), 2) == 0)
		{
			if (TryParseHex(iValue, candidate + 2) == PARSERESULT_GOOD)
			{
				return VARTYPE_Int32;
			}

			if (TryParseHex(i64Value, candidate + 2) == PARSERESULT_GOOD)
			{
				return VARTYPE_Int64;
			}
		}

		return VARTYPE_Bad;
	}

	bool TryGetDigit(OUT int32& value, char c)
	{
		if (c >= '0' && c <= '9')
		{
			value = c - '0';
			return true;
		}
		else
		{
			return false;
		}
	}

	bool IsDigit(char c)
	{
		int value;
		return TryGetDigit(OUT value, c);
	}

	bool TryGetHex(OUT int32& value, char c)
	{
		if (c >= '0' && c <= '9')
		{
			value = c - '0';
		}
		else if (c >= 'A' && c <= 'F')
		{
			value = c - 'A' + 10;
		}
		else if (c >= 'a' && c <= 'f')
		{
			value = c - 'a' + 10;
		}
		else
		{
			return false;
		}

		return true;
	}

	cstr VarTypeName(VARTYPE type)
	{
		switch (type)
		{
		case VARTYPE_Int32:	return ("Int32");
		case VARTYPE_Int64: return ("Int64");
		case VARTYPE_Float32: return ("Float32");
		case VARTYPE_Float64: return ("Float64");
		case VARTYPE_Bool: return ("Boolean32");
		case VARTYPE_Derivative: return ("Derivative");
		case VARTYPE_Pointer: return ("Pointer");
		default: return ("Unknown variable type");
		}
	}

	bool IsNAN(float x)
	{
		long xbits = *(long*)&x;
		return ((xbits & 0x7f800000L) == 0x7f800000L) && ((xbits & 0x007fffffL) != 0000000000L);
	}

	bool IsInfinite(float x)
	{
		long xbits = *(long*)&x;
		return ((xbits & 0x7f800000L) == 0x7f800000L) && ((xbits & 0x007fffffL) == 0000000000L);
	}

	bool IsNAN(double x)
	{
      union { uint64 u; double f; } ieee754;
      ieee754.f = x;
      return ((unsigned)(ieee754.u >> 32) & 0x7fffffff) +  ((unsigned)ieee754.u != 0) > 0x7ff00000;
	}

	bool IsInfinite(double x)
	{
      union { uint64 u; double f; } ieee754;
      ieee754.f = x;
      return ((unsigned)(ieee754.u >> 32) & 0x7fffffff) == 0x7ff00000 && ((unsigned)ieee754.u == 0);
	}

	bool IsDecimal(cstr x)
	{
		if (x == NULL || x[0] == 0) return false;

		enum STATE {STATE_START, STATE_MANTISSA_BEFORE_POINT, STATE_MANTISSA_AFTER_POINT, STATE_EXPONENT_START, STATE_EXPONENT_BEFORE_POINT, STATE_EXPONENT_AFTER_POINT} state;
		state = STATE_START;

		for(cstr p = x; *p != 0; p++)
		{
			char c = *p;
			switch(state)
			{
			case STATE_START:
				if (IsDigit(c))
				{
					state = STATE_MANTISSA_BEFORE_POINT;
				}
				else if (c == '+' || c == '-')
				{
					state = STATE_MANTISSA_BEFORE_POINT;
				}
				else if (c == '.')
				{
					state = STATE_MANTISSA_AFTER_POINT;
				}
				else if (c == 'E' || c == 'e')
				{
					state = STATE_EXPONENT_START;
				}
				else
				{
					return false;
				}
				break;
			case STATE_MANTISSA_BEFORE_POINT:
				if (IsDigit(c))
				{
					// still reading the mantissa
				}
				else if (c == '.')
				{
					state = STATE_MANTISSA_AFTER_POINT;
				}
				else if (c == 'E' || c == 'e')
				{
					state = STATE_EXPONENT_START;
				}
				else
				{
					return false;
				}
				break;
			case STATE_EXPONENT_START:
				if (IsDigit(c))
				{
					state = STATE_EXPONENT_BEFORE_POINT;
				}
				else if (c == '+' || c == '-')
				{
					state = STATE_EXPONENT_BEFORE_POINT;
				}
				else if (c == '.')
				{
					state = STATE_EXPONENT_AFTER_POINT;
				}
				else
				{
					return false;
				}
				break;
			case STATE_EXPONENT_BEFORE_POINT:
				if (IsDigit(c))
				{
					// still reading the exponent before the point
				}
				else if (c == '.')
				{
					state = STATE_EXPONENT_AFTER_POINT;
				}
				else
				{
					return false;
				}
				break;
			case STATE_EXPONENT_AFTER_POINT:
				if (IsDigit(c))
				{
					// still reading the exponent after the point
				}
				else
				{
					return false;
				}
				break;
         case STATE_MANTISSA_AFTER_POINT:
            break;
			}
        
		}

		return true;
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

	PARSERESULT TryParseFloat(OUT float32& value, IN cstr s)
	{
		PARSERESULT a1 =_TryParseDecimalSequence<float32>(OUT value, s);
		if (a1 == PARSERESULT_BAD_DECIMAL_DIGIT)
		{
			return _TryParseExponentForm<float32>(OUT value, IN s);
		}

		return a1;
	}

	PARSERESULT TryParseFloat(OUT float64& value, IN cstr s)
	{
		PARSERESULT a1 =_TryParseDecimalSequence<float64>(OUT value, s);
		if (a1 == PARSERESULT_BAD_DECIMAL_DIGIT)
		{
			return _TryParseExponentForm<float64>(OUT value, IN s);
		}

		return a1;
	}

	PARSERESULT TryParseBoolean(OUT int32& value, IN cstr valueLiteral)
	{
		if (AreEqual(valueLiteral, ("true")) || AreEqual(valueLiteral, ("1")))
		{
			value = 1;
			return PARSERESULT_GOOD;
		}

		if (AreEqual(valueLiteral, ("false")) || AreEqual(valueLiteral, ("0")))
		{
			value = 0;
			return PARSERESULT_GOOD;
		}

		return PARSERESULT_UNHANDLED_TYPE;
	}

	PARSERESULT TryParseDecimal(OUT int32& value, IN cstr valueLiteral)
	{
		bool isPositive = true;
		cstr valueDigits = valueLiteral;

		if (valueLiteral[0] == '-')
		{
			isPositive = false;
			valueDigits++;
		}
		else if (valueLiteral[0] == '+')
		{
			valueDigits++;
		}

		int32 x = 0;
		int32 exponent = 1;

		int len = StringLength(valueDigits);

		for(int i = len-1; i >= 0; --i)
		{
			int32 g;
			if (TryGetDigit(OUT g, valueDigits[i]))
			{
				int32 oldx = x;
				x += g * exponent;

				if ((x & 0x80000000) != 0)
				{
					if (!isPositive)
					{
						// Edge case, int.min = -int.max -1, which is the case when the sign bit flips, and the rest of the bits are zero
						for(int j = i-1; j >= 0; --i)
						{
							if (valueDigits[j] != '0')
							{
								return PARSERESULT_OVERFLOW;
							}
						}

						value = 0x80000000;
						return x == 0x80000000 ? PARSERESULT_GOOD : PARSERESULT_OVERFLOW;
					}
					// The sign bit flipped, so we overflowed
					return PARSERESULT_OVERFLOW;
				}
			}
			else
			{
				return PARSERESULT_BAD_DECIMAL_DIGIT;
			}

			exponent *= 10;
		}	

		value = isPositive ? x : -x;
		return PARSERESULT_GOOD;
	}

	PARSERESULT TryParseDecimal(OUT int64& value, IN cstr valueLiteral)
	{
		bool isPositive = true;
		cstr valueDigits = valueLiteral;

		if (valueLiteral[0] == '-')
		{
			isPositive = false;
			valueDigits++;
		}
		else if (valueLiteral[0] == '+')
		{
			valueDigits++;
		}

		int64 x = 0;
		int64 exponent = 1;

		int len = StringLength(valueDigits);

		for(int i = len-1; i >= 0; --i)
		{
			int32 g;
			if (TryGetDigit(OUT g, valueDigits[i]))
			{
				int64 oldx = x;
				x += ((int64)g) * exponent;

				if ((x & 0x8000000000000000) != 0)
				{
					if (!isPositive)
					{
						// Edge case, int.min = -int.max -1, which is the case when the sign bit flips, and the rest of the bits are zero
						for(int j = i-1; j >= 0; --i)
						{
							if (valueDigits[j] != '0')
							{
								return PARSERESULT_OVERFLOW;
							}
						}

						value = 0x8000000000000000;
						return x == 0x8000000000000000 ? PARSERESULT_GOOD : PARSERESULT_OVERFLOW;
					}
					// The sign bit flipped, so we overflowed
					return PARSERESULT_OVERFLOW;
				}
			}
			else
			{
				return PARSERESULT_BAD_DECIMAL_DIGIT;
			}

			exponent *= 10;
		}	

		value = isPositive ? x : -x;
		return PARSERESULT_GOOD;
	}

	PARSERESULT TryParse(OUT VariantValue& value, VARTYPE type, IN cstr valueLiteral)
	{
		if (Compare(valueLiteral, ("0x"), 2) == 0)
		{
			switch (type)
			{
			case VARTYPE_Int32:
				return TryParseHex(OUT value.int32Value, IN valueLiteral + 2);
			case VARTYPE_Int64:
				return TryParseHex(OUT value.int64Value, IN valueLiteral + 2);
			case VARTYPE_Float32:
				return PARSERESULT_HEX_FOR_FLOAT;
			case VARTYPE_Float64:
				return PARSERESULT_HEX_FOR_FLOAT;
			default:
				return PARSERESULT_UNHANDLED_TYPE;
			}
		}

		switch(type)
		{
		case VARTYPE_Bool:
			return TryParseBoolean(OUT value.int32Value, IN valueLiteral);
		case VARTYPE_Int32:
			return TryParseDecimal(OUT value.int32Value, IN valueLiteral);
		case VARTYPE_Int64:
			return TryParseDecimal(OUT value.int64Value, IN valueLiteral);
		case VARTYPE_Float32:
			return TryParseFloat(OUT value.floatValue, IN valueLiteral);
		case VARTYPE_Float64:
			return TryParseFloat(OUT value.doubleValue, IN valueLiteral);
		default:
			return PARSERESULT_UNHANDLED_TYPE;
		}
	}

	bool ContainsPoint(cstr s)
	{
		for(cstr p = s; *p != 0; p++)
		{
			if (*p == '.')
			{
				return true;
			}
		}

		return false;
	}

	// Attempt to parse a number in the format {mantissa}E{exponent}
	PARSERESULT TryParseExponentForm(OUT double& y, cstr s)
	{
		return _TryParseExponentForm(OUT y, IN s);
	}

	// Attempt to parse a number in the format {mantissa}E{exponent}
	PARSERESULT TryParseExponentForm(OUT float& y, cstr s)
	{
		return _TryParseExponentForm(OUT y, IN s);
	}
}} // Rococo::Compiler