namespace
{ 
	using namespace Rococo;
	using namespace Rococo::IO;
	using namespace Rococo::Sexy;

	// To use this function implement a parser object and pass it as an argument.
	// An example is LINE_PARSER implemented in void Rococo::IO::ParseTabbedCSVString(cstr csvString, ICSVLineParser& lineParser)
	template<class PARSER> void TokenizeTabbedCSVFile(cstr csvString, PARSER& tokenParser)
	{
		int row = 1;
		int column = 1;

		Vec2i cursor = { 1,1 };

		enum EState
		{
			STATE_EXPECTING_TOKEN,
			STATE_READING_SIMPLE_TOKEN,
			STATE_READING_STRING_LITERAL,
			STATE_READING_STRING_LITERAL_ESCAPE,
			STATE_TERMINATING_STRING_LITERAL
		} state = STATE_EXPECTING_TOKEN;

		tokenParser.row = 0;

		cstr p = csvString;
		for (; ; p++)
		{
			char c = *p;

			switch (state)
			{
			case STATE_EXPECTING_TOKEN:
				if (c == 0)
				{
					goto end;
				}
				else if (c == '\t')
				{
					column++;
					cursor.x++;
					tokenParser.Submit(row, column);
				}
				else if (c == '\n')
				{
					column = 1;
					cursor.x = 1;
					row++;
					cursor.y++;

					tokenParser.row = row;
					tokenParser.OnBlankLine(cursor);
				}
				else if (c <= 32)
				{
					tokenParser.OnBadChar(cursor, c);
				}
				else if (c == '"')
				{
					state = STATE_READING_STRING_LITERAL;
					cursor.x++;
				}
				else
				{
					cursor.x++;
					state = STATE_READING_SIMPLE_TOKEN;
					tokenParser.StartRaw(p);
				}
				break;
			case STATE_READING_SIMPLE_TOKEN:
				if (c == 0)
				{
					goto end;
				}
				else if (c == '\t')
				{
					tokenParser.EndRaw(p);
					tokenParser.Submit(row, column);

					cursor.x++;
					column++;

					state = STATE_EXPECTING_TOKEN;
				}
				else if (c == '\n')
				{
					tokenParser.row = row;
					tokenParser.EndRaw(p);
					tokenParser.Submit(row, column);
					tokenParser.OnNewLine();

					column = 1;
					cursor.x = 1;
					row++;
					cursor.y++;

					state = STATE_EXPECTING_TOKEN;
				}
				else if (c <= 32)
				{
					tokenParser.OnBadChar(cursor, c);
					return;
				}
				else if (c == '"')
				{
					tokenParser.OnBadChar(cursor, c);
					return;
				}
				else
				{
					cursor.x++;
				}
				break;
			case STATE_READING_STRING_LITERAL:
				if (c == 0)
				{
					tokenParser.OnBadChar(cursor, c);
					return;
				}
				else if (c == '"')
				{
					tokenParser.Submit(row, column);
					cursor.x++;

					state = STATE_TERMINATING_STRING_LITERAL;
				}
				else if (c == '\n')
				{
					tokenParser.row = row;
					tokenParser.AppendStringLiteral('\n');
					cursor.x = 1;
					cursor.y++;
				}
				else if (c <= 32)
				{
					tokenParser.AppendStringLiteral(c);
					cursor.x++;
				}
				else if (c == '\\')
				{
					state = STATE_READING_STRING_LITERAL_ESCAPE;
					cursor.x++;
				}
				else
				{
					tokenParser.AppendStringLiteral(c);
					cursor.x++;
				}
				break;
			case STATE_READING_STRING_LITERAL_ESCAPE:
				if (c == 0)
				{
					tokenParser.OnBadChar(cursor, c);
					return;
				}
				else if (c == '"')
				{
					tokenParser.AppendStringLiteral('"');
					state = STATE_READING_STRING_LITERAL;
					cursor.x++;
				}
				else if (c == '\\')
				{
					tokenParser.AppendStringLiteral('\\');
					state = STATE_READING_STRING_LITERAL;
					cursor.x++;
				}
				else
				{
					tokenParser.OnBadChar(cursor, c);
				}
				break;
			case STATE_TERMINATING_STRING_LITERAL:
				if (c == 0)
				{
					goto end;
				}
				else if (c == '\t')
				{
					cursor.x++;
					column++;
					state = STATE_EXPECTING_TOKEN;
				}
				else if (c == '\n')
				{
					tokenParser.row = row;
					cursor.x = 1;
					cursor.y++;
					column = 1;
					row++;
					state = STATE_EXPECTING_TOKEN;
					tokenParser.OnNewLine();
				}
				else
				{
					tokenParser.OnBadChar(cursor, c);
				}
				break;
			}
		}

	end:
		return;
	}
};
