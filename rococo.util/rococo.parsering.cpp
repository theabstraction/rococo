#include <rococo.parse.h>
#include <rococo.strings.h>

namespace Rococo
{
	namespace Parse
	{
		PARSERESULT TryParseHex(int32& value, cstr hexDigits)
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

		PARSERESULT TryParseHex(int64& value, cstr hexDigits)
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
	}
}