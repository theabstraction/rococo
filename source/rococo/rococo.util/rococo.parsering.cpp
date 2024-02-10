#define ROCOCO_API __declspec(dllexport)
#include <rococo.parse.h>
#include <rococo.strings.h>
#include <rococo.validators.h>
#include <memory.h>

using namespace Rococo;
using namespace Rococo::Parse;
using namespace Rococo::Strings;

namespace Rococo::Strings
{
	ROCOCO_API int32 StringLength(const char* s);
}

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
		DoubleBits X, Y;
		X.DoubleValue = GetInfinity<double>();
		Y.DoubleValue = x;

		return X.HexValue == Y.HexValue;
	}

	float IsInfinity(float x)
	{
		FloatBits X, Y;
		X.FloatValue = GetInfinity<float>();
		Y.FloatValue = x;

		return X.HexValue == Y.HexValue;
	}

	bool IsNegativeInfinity(double x)
	{
		DoubleBits X, Y;
		X.DoubleValue = GetNegativeInfinity<double>();
		Y.DoubleValue = x;

		return X.HexValue == Y.HexValue;
	}

	bool IsNegativeInfinity(float x)
	{
		FloatBits X, Y;
		X.FloatValue = GetNegativeInfinity<float>();
		Y.FloatValue = x;

		return X.HexValue == Y.HexValue;
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

		for (int i = startPos; s[i] != 0; i++)
		{
			if (s[i] == '.')
			{
				dotPos = i;
				break;
			}
		}

		T base = 1.0;

		y = 0;

		for (int j = dotPos - 1; j >= startPos; j--)
		{
			char c = s[j];
			if (c < '0' || c > '9')
			{
				return PARSERESULT_BAD_DECIMAL_DIGIT;
			}
		}

		if (s[dotPos] != 0)
		{
			for (int j = dotPos + 1; s[j] != 0; j++)
			{
				char c = s[j];
				if (c < '0' || c > '9')
				{
					return PARSERESULT_BAD_DECIMAL_DIGIT;
				}
			}
		}

		bool wasValued = false;

		for (int j = dotPos - 1; j >= startPos; j--)
		{
			char c = s[j];
			T unit = (T)(c - '0');
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
			for (int j = dotPos + 1; s[j] != 0; j++)
			{
				base *= (T)0.1;

				char c = s[j];

				T unit = (T)(c - '0');
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
		enum { MAX_CHARS = 64 };
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

	bool TrySplitByChar(IN char splitChar1, IN char splitChar2, IN cstr s, OUT cstr& start, OUT cstr& end, REF char* tempBuffer, IN size_t charsInBuffer)
	{
		int len = Rococo::Strings::StringLength(s);
		if (len >= (int)charsInBuffer)
		{
			return false;
		}

		memcpy_s(tempBuffer, sizeof(char) * charsInBuffer, s, sizeof(char) * len);
		tempBuffer[len] = 0;

		for (int i = 0; i < len; ++i)
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

namespace Rococo::Parse
{
	using namespace Rococo::Strings;

	ROCOCO_API bool TryGetDigit(OUT int32& value, char c)
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

	ROCOCO_API bool IsDigit(char c)
	{
		int value;
		return TryGetDigit(OUT value, c);
	}

	ROCOCO_API bool TryGetHex(OUT int32& value, char c)
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

	ROCOCO_API PARSERESULT TryParseHex(int32& value, cstr hexDigits)
	{
		int len = StringLength(hexDigits);
		if (len == 0 || len > 8)
		{
			return PARSERESULT_HEXADECIMAL_INCORRECT_NUMBER_OF_DIGITS;
		}

		int32 x = 0;
		int32 shift = 0;
		for (int i = len - 1; i >= 0; --i)
		{
			int32 g;
			if (TryGetHex(OUT g, hexDigits[i]))
			{
				x |= (((int64)g) << shift);
			}
			else
			{
				return PARSERESULT_HEXADECIMAL_BAD_CHARACTER;
			}

			shift += 4;
		}

		value = x;
		return PARSERESULT_GOOD;
	}

	ROCOCO_API PARSERESULT TryParseHex(int64& value, cstr hexDigits)
	{
		int len = (int)StringLength(hexDigits);
		if (len == 0 || len > 16)
		{
			return PARSERESULT_HEXADECIMAL_INCORRECT_NUMBER_OF_DIGITS;
		}

		int64 x = 0;
		int32 shift = 0;
		for (int i = len - 1; i >= 0; --i)
		{
			int32 g;
			if (TryGetHex(OUT g, hexDigits[i]))
			{
				x |= (((int64)g) << shift);
			}
			else
			{
				return PARSERESULT_HEXADECIMAL_BAD_CHARACTER;
			}

			shift += 4;
		}

		value = x;
		return PARSERESULT_GOOD;
	}

	ROCOCO_API bool IsNAN(float x)
	{
		long xbits = *(long*)&x;
		return ((xbits & 0x7f800000L) == 0x7f800000L) && ((xbits & 0x007fffffL) != 0000000000L);
	}

	ROCOCO_API bool IsInfinite(float x)
	{
		long xbits = *(long*)&x;
		return ((xbits & 0x7f800000L) == 0x7f800000L) && ((xbits & 0x007fffffL) == 0000000000L);
	}

	ROCOCO_API bool IsNAN(double x)
	{
		union { uint64 u; double f; } ieee754;
		ieee754.f = x;
		return ((unsigned)(ieee754.u >> 32) & 0x7fffffff) + ((unsigned)ieee754.u != 0) > 0x7ff00000;
	}

	ROCOCO_API bool IsInfinite(double x)
	{
		union { uint64 u; double f; } ieee754;
		ieee754.f = x;
		return ((unsigned)(ieee754.u >> 32) & 0x7fffffff) == 0x7ff00000 && ((unsigned)ieee754.u == 0);
	}

	ROCOCO_API bool IsDecimal(cstr x)
	{
		if (!x || x[0] == 0) return false;

		enum STATE { STATE_START, STATE_MANTISSA_BEFORE_POINT, STATE_MANTISSA_AFTER_POINT, STATE_EXPONENT_START, STATE_EXPONENT_BEFORE_POINT, STATE_EXPONENT_AFTER_POINT } state;
		state = STATE_START;

		for (cstr p = x; *p != 0; p++)
		{
			char c = *p;
			switch (state)
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

	ROCOCO_API PARSERESULT TryParseFloat(OUT float32& value, IN cstr s)
	{
		PARSERESULT a1 = _TryParseDecimalSequence<float32>(OUT value, s);
		if (a1 == PARSERESULT_BAD_DECIMAL_DIGIT)
		{
			return _TryParseExponentForm<float32>(OUT value, IN s);
		}

		return a1;
	}

	ROCOCO_API PARSERESULT TryParseFloat(OUT float64& value, IN cstr s)
	{
		PARSERESULT a1 = _TryParseDecimalSequence<float64>(OUT value, s);
		if (a1 == PARSERESULT_BAD_DECIMAL_DIGIT)
		{
			return _TryParseExponentForm<float64>(OUT value, IN s);
		}

		return a1;
	}

	ROCOCO_API PARSERESULT TryParseBoolean(OUT int32& value, IN cstr valueLiteral)
	{
		if (Eq(valueLiteral, "true") || Eq(valueLiteral, "1"))
		{
			value = 1;
			return PARSERESULT_GOOD;
		}

		if (Eq(valueLiteral, "false") || Eq(valueLiteral, "0"))
		{
			value = 0;
			return PARSERESULT_GOOD;
		}

		return PARSERESULT_UNHANDLED_TYPE;
	}

	ROCOCO_API PARSERESULT TryParseDecimal(OUT int32& value, IN cstr valueLiteral)
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

		for (int i = len - 1; i >= 0; --i)
		{
			int32 g;
			if (TryGetDigit(OUT g, valueDigits[i]))
			{
				x += g * exponent;

				if ((x & 0x80000000) != 0)
				{
					if (!isPositive)
					{
						// Edge case, int.min = -int.max -1, which is the case when the sign bit flips, and the rest of the bits are zero
						for (int j = i - 1; j >= 0; --i)
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

	ROCOCO_API PARSERESULT TryParseDecimal(OUT int64& value, IN cstr valueLiteral)
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

		for (int i = len - 1; i >= 0; --i)
		{
			int32 g;
			if (TryGetDigit(OUT g, valueDigits[i]))
			{
				x += ((int64)g) * exponent;

				if ((x & 0x8000000000000000) != 0)
				{
					if (!isPositive)
					{
						// Edge case, int.min = -int.max -1, which is the case when the sign bit flips, and the rest of the bits are zero
						for (int j = i - 1; j >= 0; --i)
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

	ROCOCO_API bool ContainsPoint(cstr s)
	{
		for (cstr p = s; *p != 0; p++)
		{
			if (*p == '.')
			{
				return true;
			}
		}

		return false;
	}

	// Attempt to parse a number in the format {mantissa}E{exponent}
	ROCOCO_API PARSERESULT TryParseExponentForm(OUT double& y, cstr s)
	{
		return _TryParseExponentForm(OUT y, IN s);
	}

	// Attempt to parse a number in the format {mantissa}E{exponent}
	ROCOCO_API PARSERESULT TryParseExponentForm(OUT float& y, cstr s)
	{
		return _TryParseExponentForm(OUT y, IN s);
	}
} // Rococo::Parse

namespace Rococo::Validators
{
	template<class VALUE_TYPE>
	struct Impl_AllPrimitivesAreValid : IValueValidator<VALUE_TYPE>
	{
		void ThrowIfBad(VALUE_TYPE value, EValidationPurpose purpose) const override
		{
			UNUSED(value)
			UNUSED(purpose)
		}
	};

	ROCOCO_API IValueValidator<int32>& AllInt32sAreValid() { static Impl_AllPrimitivesAreValid<int32> impl; return impl; }
	ROCOCO_API IValueValidator<int64>& AllInt64sAreValid() { static Impl_AllPrimitivesAreValid<int64> impl; return impl; }
	ROCOCO_API IValueValidator<uint32>& AllUInt32sAreValid() { static Impl_AllPrimitivesAreValid<uint32> impl; return impl; }
	ROCOCO_API IValueValidator<uint64>& AllUInt64sAreValid() { static Impl_AllPrimitivesAreValid<uint64> impl; return impl; }
	ROCOCO_API IValueValidator<bool>& AllBoolsAreValid() { static Impl_AllPrimitivesAreValid<bool> impl; return impl; }
	ROCOCO_API IValueValidator<float>& AllFloatsAreValid() { static Impl_AllPrimitivesAreValid<float> impl; return impl; }
	ROCOCO_API IValueValidator<double>& AllDoublesAreValid() { static Impl_AllPrimitivesAreValid<double> impl; return impl; }

	ROCOCO_API IValueFormatter<int32>& Int32Decimals()
	{
		struct Impl : IValueFormatter<int32>
		{
			void Format(char* buffer, size_t capacity, int32 value) const override
			{
				SafeFormat(buffer, capacity, "%d", value);
			}
		};

		static Impl anon;
		return anon;

	}

	ROCOCO_API IValueFormatter<int64>& Int64Decimals()
	{
		struct Impl : IValueFormatter<int64>
		{
			void Format(char* buffer, size_t capacity, int64 value) const override
			{
				SafeFormat(buffer, capacity, "%lld", value);
			}
		};

		static Impl anon;
		return anon;

	}

	ROCOCO_API IValueFormatter<uint32>& Uint32Decimals()
	{
		struct Impl : IValueFormatter<uint32>
		{
			void Format(char* buffer, size_t capacity, uint32 value) const override
			{
				SafeFormat(buffer, capacity, "%u", value);
			}
		};

		static Impl anon;
		return anon;

	}

	ROCOCO_API IValueFormatter<uint64>& Uint64Decimals()
	{
		struct Impl : IValueFormatter<uint64>
		{
			void Format(char* buffer, size_t capacity, uint64 value) const override
			{
				SafeFormat(buffer, capacity, "%llu", value);
			}
		};

		static Impl anon;
		return anon;

	}

	ROCOCO_API IValueFormatter<bool>& BoolFormatter()
	{
		struct Impl : IValueFormatter<bool>
		{
			void Format(char* buffer, size_t capacity, bool value) const override
			{
				SafeFormat(buffer, capacity, "%s", value ? "true" : "false");
			}
		};

		static Impl anon;
		return anon;

	}

	ROCOCO_API IValueFormatter<float>& FloatDecimals()
	{
		struct Impl : IValueFormatter<float>
		{
			void Format(char* buffer, size_t capacity, float value) const override
			{
				SafeFormat(buffer, capacity, "%f", value);
			}
		};

		static Impl anon;
		return anon;
	}

	ROCOCO_API IValueFormatter<double>& DoubleDecimals()
	{
		struct Impl : IValueFormatter<double>
		{
			void Format(char* buffer, size_t capacity, double value) const override
			{
				SafeFormat(buffer, capacity, "%llg", value);
			}
		};

		static Impl anon;
		return anon;
	}
}