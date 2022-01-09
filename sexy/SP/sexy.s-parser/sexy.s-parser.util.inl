namespace
{
	sexstring CreateSexString(const SEXCHAR* src, int32 length)
	{
		int32 nBytes = sizeof(int32) + sizeof(SEXCHAR) * (length+1);
		sexstring s = (sexstring) new char[nBytes];
		s->Length = length;

		memcpy_s(s->Buffer, sizeof(SEXCHAR) * (length+1), src, sizeof(SEXCHAR) * length);

		s->Buffer[length] = 0;
		return s;
	}

	void FreeSexString(sexstring s)
	{
		char* buf = (char*) s;
		delete[] buf;
	}

	bool TryGetSexCharFromHex(SEXCHAR& value, SEXCHAR hex)
	{
		if (hex >= (SEXCHAR) '0' && hex <= (SEXCHAR) '9')
		{
			value = hex -(SEXCHAR) '0';
			return true;
		}

		if (hex >= (SEXCHAR) 'A' && hex <= (SEXCHAR) 'F')
		{
			value = 10 + hex -(SEXCHAR) 'A';
			return true;
		}

		if (hex >= (SEXCHAR) 'a' && hex <= (SEXCHAR) 'f')
		{
			value = 10 + hex -(SEXCHAR) 'a';
			return true;
		}

		return false;
	}

	bool ParseEscapeCharacter(SEXCHAR& finalChar, SEXCHAR c)
	{
		switch(c)
		{
		case '\\': // \\ maps to single backslash
			finalChar = (SEXCHAR)'\\';
			break;
		case '"': // \" maps to "
			finalChar =  (SEXCHAR)'"';
			break;
		case 't': // \t maps to horizontal tab
			finalChar =  (SEXCHAR)'\t';
			break;
		case 'r': // \r maps to linefeed
			finalChar =  (SEXCHAR)'\r';
			break;
		case 'n': // \n maps to newline
			finalChar =  (SEXCHAR)'\n';
			break;
		case '0': // \0 maps to character null
			finalChar =  (SEXCHAR)'\0';
			break;
		case 'a': // \a maps to bell (alert)
			finalChar =  (SEXCHAR)'\a';
			break;
		case 'b': // \b maps to backspace
			finalChar =  (SEXCHAR)'\b';
			break;
		case 'f': // \b maps to formfeed
			finalChar =  (SEXCHAR)'\f';
			break;
		case 'v': // \b maps to vertical tab
			finalChar =  (SEXCHAR)'\v';
			break;
		case '\'': // \' maps to '
			finalChar =  (SEXCHAR)'\'';
			break;
		case '?': // \? maps to ?'
			finalChar =  (SEXCHAR)'?';
			break;
		default:
			return false;
		}

		return true;
	}

	bool TryParseSexHex(SEXCHAR& finalChar, const SEXCHAR* s)
	{
		if (sizeof(SEXCHAR) == 1)
		{			
			SEXCHAR c16;
			SEXCHAR c1;
			if (!TryGetSexCharFromHex(c16, s[0])) return false;
			if (!TryGetSexCharFromHex(c1, s[1]))  return false;
			finalChar = c1 + (c16 << 4);
		}
		else if (sizeof(SEXCHAR) == 2)
		{
			// \X means insert byte by four hex digits (16-bit UNICODE)
			SEXCHAR c4096;
			SEXCHAR c256;
			SEXCHAR c16;
			SEXCHAR c1;

			if (!TryGetSexCharFromHex(c4096, s[0])) return false;
			if (!TryGetSexCharFromHex(c256,  s[1])) return false;
			if (!TryGetSexCharFromHex(c16,   s[2])) return false;
			if (!TryGetSexCharFromHex(c1,    s[3])) return false;
			finalChar = c1 + (c16 << 4) + (c256 << 8) + (c4096 << 12);
		}

		return true;
	}
}