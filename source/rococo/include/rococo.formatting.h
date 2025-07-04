#pragma once

namespace Rococo::Formatting
{
	enum class ETryParseResultCode
	{
		Success,
		NullString,
		BlankString,
		Overflow
	};

	template<class T>
	struct TryParseResult
	{
		ETryParseResultCode code;
		T Value;
		int Position = 0;
	};

	bool ToAscii(int32 iValue, int radix, bool addThousandsMarker, char thousandMarkerChar, char* buffer, size_t capacity);
	bool ToAscii(int64 iValue, int radix, bool addThousandsMarker, char thousandMarkerChar, char* buffer, size_t capacity);
	TryParseResult<int32> TryParseInt32FromDecimalStringSkippingCetera(cstr asciiText);
	TryParseResult<int64> TryParseInt64FromDecimalStringSkippingCetera(cstr asciiText);
}