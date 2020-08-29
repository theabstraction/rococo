namespace
{
	using namespace Rococo;

	struct CSVCursor
	{
		const char* start; // The first character to be parsed in the current line
		const char* end; // The last valid character of the current line, so if our line is alpha!\r\n or alpha!\n then the last valid character points to !
		const char* finale; // Just beond the last character in the CSV file
		int lineNumber;
		int tokenIndex; // Gives the current column number
		size_t Length() const { return end - start + 1; }
	};

	struct SkipItem {};

	inline SkipItem Skip() { return SkipItem(); }

	struct ValidateItem
	{
		ValidateItem(const char* _item) : item(_item) {}
		const char* item;
	};

	struct StringBuffer
	{
		char* buffer;
		size_t bufferCapacity;

		StringBuffer(char* _buffer, size_t _bufferCapacity) : buffer(_buffer), bufferCapacity(_bufferCapacity) {}
	};

	class CSVStream
	{
	private:
		CSVCursor cursor;
		const char* buffer;
		const size_t nBytes;
		char filename[_MAX_PATH];
	public:
		CSVStream(cstr _filename, const char* buffer, size_t nBytes);

		CSVStream& operator >> (SkipItem& item);
		CSVStream& operator >> (ValidateItem& item);
		CSVStream& operator >> (int32& i);
		CSVStream& operator >> (float& i);
		CSVStream& operator >> (StringBuffer& s);

		bool AdvanceToNextLine();
		cstr Filename() const { return filename; }
	};

	bool AdvanceCSVToNextLine(CSVCursor& cursor)
	{
		bool inquote = false;
		char previousChar = ' ';

		cursor.tokenIndex = 0;

		// cursor.end will initially be pointing to the last character of the current line, followed by an optional \r and a \n, or followed by EOF

		const char* p = cursor.end + 1;

		if (p == cursor.finale) return false;

		if (*p == '\r') p++;

		if (p == cursor.finale) return false;

		if (*p == '\n') p++;

		if (p == cursor.finale) return false;

		cursor.lineNumber++;

		cursor.start = p;

		for (; p < cursor.finale; p++)
		{
			char c = *p;

			if (c == '"')
			{
				if (inquote)
				{
					inquote = false;
				}
				else
				{
					inquote = true;
				}
			}
			else
			{
				if (!inquote)
				{
					if (c == '\n')
					{
						if (previousChar == '\r')
						{
							cursor.end = p - 2;
							return true;
						}
						else
						{
							cursor.end = p - 1;
							return true;
						}
					}
				}
			}

			previousChar = c;
		}

		cursor.end = cursor.finale;
		return true;
	}

	void SetCSVToStart(const char* buffer, size_t nBytes, CSVCursor& cursor)
	{
		bool inquote = false;

		const char* p = (const char*)buffer;
		const char* finale = p + nBytes;

		cursor.lineNumber = 0;
		cursor.tokenIndex = 0;
		cursor.finale = finale;

		// Skip first new lines, if any
		while (p < finale)
		{
			if (*p == '\r')
			{
			}
			else if (*p == '\n')
			{
				cursor.lineNumber++;
			}
			else
			{
				break;
			}
			p++;
		}

		if (p >= finale)
		{
			cursor.start = cursor.end = nullptr;
			return;
		}

		cursor.start = p;

		char previousChar = ' ';

		for (; p < finale; p++)
		{
			char c = *p;

			if (c == '"')
			{
				if (inquote)
				{
					inquote = false;
				}
				else
				{
					inquote = true;
				}
			}
			else
			{
				if (!inquote)
				{
					if (c == '\n')
					{
						if (previousChar == '\r')
						{
							cursor.end = p - 2;
							return;
						}
						else
						{
							cursor.end = p - 1;
							return;
						}
					}
				}
			}

			previousChar = c;
		}

		cursor.end = cursor.finale;
	}

	void GetCSVItem(cstr fullname, CSVCursor& cursor, char* buffer, size_t bufferLen)
	{
		if (cursor.start == NULL)
		{
			Throw(0, "Expecting CSV item in %s, but the csv cursor was invalid at line %d", fullname, cursor.lineNumber);
		}

		char previousChar = ' ';

		const char* tokenStart = cursor.start;
		const char* p = cursor.start;
		const char* end = cursor.end;

		if (p > end)
		{
			Throw(0, "Expecting CSV item in %s line %d item %d", fullname, cursor.lineNumber, cursor.tokenIndex);
		}

		bool inquote = *p == '"';

		if (inquote)
		{
			p++;

			for (; p < end; p++)
			{
				char c = *p;
				if (c == '"')
				{
					inquote = false;
					break;
				}
			}

			if (inquote)
			{
				Throw(0, "Quotation not terminated in %s line %d", fullname, cursor.lineNumber);
			}
		}

		const char* next = NULL;

		if (p == end)
		{
			p++;
		}
		else
		{
			for (; p <= end; p++)
			{
				char c = *p;
				if (c == ',')
				{
					size_t len = p - tokenStart;

					if (len >= bufferLen)
					{
						Throw(0, "Insufficient buffer space to copy %s line %d item %d", fullname, cursor.lineNumber, cursor.tokenIndex);
					}

					memcpy_s(buffer, bufferLen, tokenStart, len);
					buffer[len] = 0;
					next = p + 1;
					break;
				}
			}
		}

		if (next == NULL)
		{
			size_t len = p - tokenStart;
			if (len >= bufferLen)
			{
				Throw(0, "Insufficient buffer space to copy %s line %d item %d", fullname, cursor.lineNumber, cursor.tokenIndex);
			}

			memcpy_s(buffer, bufferLen, tokenStart, len);
			buffer[len] = 0;
			next = p;
		}

		cursor.start = next;
		cursor.tokenIndex++;
	}

	CSVStream::CSVStream(cstr _filename, const char* _buffer, size_t _nBytes) : buffer(_buffer), nBytes(_nBytes)
	{
      StackStringBuilder sb(filename, _MAX_PATH);
		sb << _filename;
		SetCSVToStart(buffer, nBytes, cursor);
	}

	CSVStream& CSVStream::operator >> (SkipItem& item)
	{
		size_t len = cursor.Length() + 1;
		char* token = (char*)alloca(len);
		GetCSVItem(filename, cursor, token, len);
		return *this;
	}

	CSVStream& CSVStream::operator >> (ValidateItem& item)
	{
		size_t len = cursor.Length() + 1;
		char* token = (char*)alloca(len);
		GetCSVItem(filename, cursor, token, len);

		if (strcmp(token, item.item) != 0)
		{
			size_t tokenBufferLen = strlen(item.item) + 1;
			char* expectedToken = (char*)alloca(sizeof(char)* tokenBufferLen);
			Throw(0, "Expecting '%ls' in '%s' at column %d line %d", expectedToken, filename, cursor.tokenIndex, cursor.lineNumber);
		}
		return *this;
	}

	CSVStream& CSVStream::operator >> (int32& item)
	{
		size_t len = cursor.Length() + 1;
		char* token = (char*)alloca(len);
		GetCSVItem(filename, cursor, token, len);

      item = atoi(token);
		if (item == 0 && token[0] != '0')
		{
			Throw(0, "Expecting int32 in '%s' at column %d line %d", filename, cursor.tokenIndex, cursor.lineNumber);
		}
		return *this;
	}

	CSVStream& CSVStream::operator >> (float& item)
	{
		size_t len = cursor.Length() + 1;
		char* token = (char*)alloca(len);
		GetCSVItem(filename, cursor, token, len);
      item = (float)atof(token);
      if (item == 0 && token[0] != '0')
		{
			Throw(0, "Expecting float in '%s' at column %d line %d", filename, cursor.tokenIndex, cursor.lineNumber);
		}
		return *this;
	}

	CSVStream& CSVStream::operator >> (StringBuffer& item)
	{
		GetCSVItem(filename, cursor, item.buffer, item.bufferCapacity);
		return *this;
	}

	bool CSVStream::AdvanceToNextLine()
	{
		return AdvanceCSVToNextLine(cursor);
	}
}