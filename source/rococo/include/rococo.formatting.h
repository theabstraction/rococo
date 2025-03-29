#pragma once

namespace Rococo::Format
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

	bool ToAscii(int iValue, int radix, bool addThousandsMarker, char thousandMarkerChar, char* buffer, size_t capacity);
	TryParseResult<int> TryParseInt32FromDecimalStringSkippingThousandMarks(cstr asciiText);
}