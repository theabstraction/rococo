// Copyright (c)2025 Mark Anthony Taylor. Email: mark.anthony.taylor@gmail.com. All rights reserved.
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

		ROCOCO_API bool TryGetDigit(int32& value, char c);
		ROCOCO_API bool TryGetHex(int32& value, char c);
		ROCOCO_API PARSERESULT TryParseFloat(float32& value, cstr decimalDigits);
		ROCOCO_API PARSERESULT TryParseFloat(float64& value, cstr decimalDigits);
		ROCOCO_API PARSERESULT TryParseHex(int32& value, cstr hexDigits);
		ROCOCO_API PARSERESULT TryParseHex(int64& value, cstr hexDigits);
		ROCOCO_API PARSERESULT TryParseBoolean(int32& value, cstr valueLiteral);
		ROCOCO_API PARSERESULT TryParseDecimal(int32& value, cstr valueLiteral);
		ROCOCO_API PARSERESULT TryParseDecimal(int64& value, cstr valueLiteral);
		ROCOCO_API PARSERESULT TryParseExponentForm(double& y, cstr s);
		ROCOCO_API PARSERESULT TryParseExponentForm(float& y, cstr s);
		ROCOCO_API bool ContainsPoint(cstr s);
	}

}