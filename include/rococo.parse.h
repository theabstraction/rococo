#pragma once

#include <rococo.types.h>

namespace Rococo
{
	union VariantValue;
	
	namespace Parse
	{
		enum PARSERESULT
		{
			PARSERESULT_GOOD,
			PARSERESULT_HEXADECIMAL_INCORRECT_NUMBER_OF_DIGITS,
			PARSERESULT_HEXADECIMAL_BAD_CHARACTER,
			PARSERESULT_OVERFLOW,
			PARSERESULT_UNDERFLOW,
			PARSERESULT_BAD_DECIMAL_DIGIT,
			PARSERESULT_HEX_FOR_FLOAT,
			PARSERESULT_UNHANDLED_TYPE
		};

		bool TryGetDigit(int32& value, char c);
		bool TryGetHex(int32& value, char c);
		PARSERESULT TryParseFloat(float32& value, cstr decimalDigits);
		PARSERESULT TryParseFloat(float64& value, cstr decimalDigits);
		PARSERESULT TryParseHex(int32& value, cstr hexDigits);
		PARSERESULT TryParseHex(int64& value, cstr hexDigits);
		PARSERESULT TryParseBoolean(int32& value, cstr valueLiteral);
		PARSERESULT TryParseDecimal(int32& value, cstr valueLiteral);
		PARSERESULT TryParseDecimal(int64& value, cstr valueLiteral);
		PARSERESULT TryParseExponentForm(double& y, cstr s);
		PARSERESULT TryParseExponentForm(float& y, cstr s);
		bool ContainsPoint(cstr s);
	}

}