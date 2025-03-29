#include <rococo.types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <rococo.formatting.h>

#pragma warning (disable: 4996)

namespace Rococo::Format
{
	bool ToAscii(int iValue, int radix, bool addThousandsMarker, char thousandMarkerChar, char* buffer, size_t capacity)
	{
		char asciiRep[40];
		if (_itoa_s(iValue, asciiRep, radix) != 0)
		{
			return false;
		}

		size_t nChars = strlen(asciiRep);

		if (!asciiRep || nChars >= capacity)
		{
			return false;
		}
		
		if (radix != 10 || addThousandsMarker == false)
		{
			strcpy(buffer, asciiRep);
			return true;
		}

		size_t nNumbers = *asciiRep == '-' ? nChars - 1 : nChars;
		size_t nTriplets = nNumbers / 3;

		cstr endPos = asciiRep + nChars;

		char* writePos = buffer;

		if (capacity < nNumbers + nTriplets + 1)
		{
			strcpy(buffer, asciiRep);
			return true;
		}
		
		size_t i = nNumbers;

		cstr readPos = asciiRep;
		if (*readPos == '-')
		{
			*writePos++ = '-';
			readPos++;
		}

		while (readPos < endPos)
		{
			*writePos++ = *readPos++;
			i--;
			if (i % 3 == 0 && *readPos != 0)
			{
				*writePos++ = thousandMarkerChar;
			}
		}

		*writePos = 0;

		return true;
	}

	constexpr int32 intMax = (int)0x7FFFFFFF;
	constexpr int32 intMin = (int)0x80000000;

	TryParseResult<int> TryParseInt32FromDecimalStringSkippingThousandMarks(cstr asciiText)
	{
		if (asciiText == nullptr)
		{
			return { ETryParseResultCode::NullString, 0, 0 };
		}

		// We expect the form [-] Ai... where Ai is a sequence of digits 0-9, up to 9 digits long. Blankspace, commas and apostrophe ' are skipped as thousand-marks, e.g the commas in 1,000,000
		if (*asciiText == 0)
		{
			return { ETryParseResultCode::BlankString, 0, 0 };
		}

		cstr src = asciiText;

		cstr beginDigits = asciiText;

		bool isNegative = *src == '-';
		if (isNegative)
		{
			src++;

			if (*src == 0)
			{
				return { ETryParseResultCode::BlankString, 0, 1 };
			}

			beginDigits = src;
		}


		int power = 0;
		int multiplier = isNegative ? -1 : 1;

		int sum = 0;
		
		cstr t = nullptr;

		while (*src++ != 0)
		{				
		}

		t = src - 1;

		while(t >= beginDigits)
		{
			char c = *t--;

			if (c >= '0' && c <= '9')
			{
				power++;

				int unit = c - '0';

				if (power >= 9)
				{
					if (unit >= 3)
					{
						return { ETryParseResultCode::Overflow, isNegative ? intMin : intMax, (int)(src - asciiText) - 1 };
					}

					if (power > 9)
					{
						return { ETryParseResultCode::Overflow, isNegative ? intMin : intMax, (int)(src - asciiText) - 1 };
					}
				}

				int unitValue = multiplier * unit;

				multiplier *= 10;

				sum += unitValue;

				if (isNegative && sum > 0)
				{
					return { ETryParseResultCode::Overflow, intMin, (int)(t - asciiText) };
				}

				if (!isNegative && sum < 0)
				{
					return { ETryParseResultCode::Overflow, intMax, (int)(t - asciiText) };
				}	
			}
			else
			{
				// Everything else is considered something to skip
			}
		}

		return  (power == 0) ? TryParseResult<int> { ETryParseResultCode::BlankString, 0, 0 } : TryParseResult<int> { ETryParseResultCode::Success, sum, 0 };
	}
}

#pragma warning (default: 4996)
