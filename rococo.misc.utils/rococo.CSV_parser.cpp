#include <rococo.api.h>
#include <rococo.io.h>
#include <rococo.csv.h>
#include <rococo.strings.h>
#include <vector>

#include <sexy.types.h>

using namespace Rococo;
using namespace Rococo::IO;

namespace
{
	using namespace Rococo::IO;

	struct CSV_SexyAssetParser : ICSVTokenParser
	{
		typedef void (CSV_SexyAssetParser::*FN_STATE)(int row, int column, cstr token);

		std::vector<int> memberIndex;
		IMemberBuilder& memberBuilder;

		FN_STATE tokenHandler = &CSV_SexyAssetParser::OnSignature;
		int currentColumn = 1;

		char archiveType[Rococo::MAX_FQ_NAME_LEN + 1]; // The local type. For example, if a Sys.Maths.Vec2 were archived, the type would be Vec2f
		char archiveName[Rococo::MAX_FQ_NAME_LEN + 1]; // The variable name. Very often 'this'. Limited to legal Sexy variable names.
		char archiveSource[Rococo::IO::MAX_PATHLEN]; // The source file wherein the local type is defined. For example Vec2 is Sys.Maths.sxy

		CSV_SexyAssetParser(IMemberBuilder& _memberBuilder):
			memberBuilder(_memberBuilder)
		{

		}

		void OnBadChar(Vec2i cursorPosition, char value) override
		{
			Throw(0, "Bad character with ascii value %u at column %d line %d", value, cursorPosition.x, cursorPosition.y);
		}

		int defRow = 1;
		int defColumn = 1;

		char memberTypeBuffer[Rococo::MAX_FQ_NAME_LEN + 1];
		char memberNameBuffer[Rococo::MAX_FQ_NAME_LEN + 1];

		bool primitiveLine = false;

		void OnMemberValueBool(int row, int column, cstr token)
		{
			if (defRow != row || defColumn != column)
			{
				Throw(0, "Expecting bool value at row %d column %d", defRow, defColumn);
			}

			bool value = Eq(token, "Y");

			memberBuilder.AddInt32Member(memberNameBuffer, value);

			defRow++;
			defColumn -= 2;

			tokenHandler = &CSV_SexyAssetParser::OnMemberDef;
		}

		void OnMemberValueInt32(int row, int column, cstr token)
		{
			if (defRow != row || defColumn != column)
			{
				Throw(0, "Expecting int32 hex value at row %d column %d", defRow, defColumn);
			}

			int32 hexValue = 0;
			sscanf_s(token, "%x", &hexValue);

			memberBuilder.AddInt32Member(memberNameBuffer, hexValue);

			defRow++;
			defColumn -= 2;

			tokenHandler = &CSV_SexyAssetParser::OnMemberDef;
		}

		void OnMemberValueInt64(int row, int column, cstr token)
		{
			if (defRow != row || defColumn != column)
			{
				Throw(0, "Expecting int64 hex value at row %d column %d", defRow, defColumn);
			}

			int64 hexValue = 0;
			sscanf_s(token, "%llx", &hexValue);

			memberBuilder.AddInt64Member(memberNameBuffer, hexValue);

			defRow++;
			defColumn -= 2;

			tokenHandler = &CSV_SexyAssetParser::OnMemberDef;
		}

		void OnMemberValueFloat(int row, int column, cstr token)
		{
			if (defRow != row || defColumn != column)
			{
				Throw(0, "Expecting float hex value at row %d column %d", defRow, defColumn);
			}

			int32 hexValue = 0;
			sscanf_s(token, "%x", &hexValue);

			float floatValue = *(float*)(&hexValue);

			memberBuilder.AddFloatMember(memberNameBuffer, floatValue);

			defRow++;
			defColumn -= 2;

			tokenHandler = &CSV_SexyAssetParser::OnMemberDef;
		}

		void OnMemberValueDouble(int row, int column, cstr token)
		{
			if (defRow != row || defColumn != column)
			{
				Throw(0, "Expecting double hex value at row %d column %d", defRow, defColumn);
			}

			int64 hexValue = 0;
			sscanf_s(token, "%llx", &hexValue);

			double doubleValue = *(double*)(&hexValue);

			memberBuilder.AddDoubleMember(memberNameBuffer, doubleValue);

			defRow++;
			defColumn -= 2;

			tokenHandler = &CSV_SexyAssetParser::OnMemberDef;
		}

		void OnMemberDerivativeSource(int row, int column, cstr token)
		{
			if (defRow != row || defColumn != column)
			{
				Throw(0, "Expecting double hex value at row %d column %d", defRow, defColumn);
			}

			memberBuilder.AddDerivativeMember(memberTypeBuffer, memberNameBuffer, token);

			defRow++;
			defColumn -= 1; // Go back 2 then forward one, as sub-members are a column advance over their parents

			tokenHandler = &CSV_SexyAssetParser::OnMemberDef;
		}

		void OnMemberName(int row, int column, cstr token)
		{
			if (defRow != row)
			{
				Throw(0, "Expecting member name at row %d column %d", defRow, defColumn);
			}

			CopyString(memberNameBuffer, sizeof memberNameBuffer, token);

			defColumn++;

			tokenHandler = &CSV_SexyAssetParser::OnMemberName;

			if (Eq(memberTypeBuffer, "D"))
			{
				primitiveLine = true;
				tokenHandler = &CSV_SexyAssetParser::OnMemberValueDouble;
			}
			else if (Eq(memberTypeBuffer, "F"))
			{
				primitiveLine = true;
				tokenHandler = &CSV_SexyAssetParser::OnMemberValueFloat;
			}
			else if (Eq(memberTypeBuffer, "I"))
			{
				primitiveLine = true;
				tokenHandler = &CSV_SexyAssetParser::OnMemberValueInt32;
			}
			else if (Eq(memberTypeBuffer, "?"))
			{
				primitiveLine = true;
				tokenHandler = &CSV_SexyAssetParser::OnMemberValueBool;
			}
			else
			{
				primitiveLine = false;
				// Value was not primitive
				tokenHandler = &CSV_SexyAssetParser::OnMemberDerivativeSource;
			}
		}

		void OnMemberDef(int row, int column, cstr token)
		{
			if (primitiveLine && defRow == row + 1)
			{
				// Human readable token at the end of a line - discard
				return;
			}

			while (column < defColumn)
			{
				// We are done building the previously defined member variable. We need to add this member to the parent member, not the previous member
				defColumn--;

				memberBuilder.ReturnToParent();
			}

			if (defRow != row || defColumn != column)
			{
				Throw(0, "Expecting member type at row %d column %d", defRow, defColumn);
			}

			CopyString(memberTypeBuffer, sizeof memberTypeBuffer, token);

			defColumn++;

			tokenHandler = &CSV_SexyAssetParser::OnMemberName;
		}

		void OnArchiveType(int row, int column, cstr token)
		{
			if (*token == 0 || row != 2 || column != 1)
			{
				Throw(0, "Expecting <archive-type> at row 2 column 1");
			}

			CopyString(archiveType, sizeof archiveType, token);
			tokenHandler = &CSV_SexyAssetParser::OnArchiveName;
		}

		void OnArchiveName(int row, int column, cstr token)
		{
			if (*token == 0 || row != 2 || column != 2)
			{
				Throw(0, "Expecting <archive-name> at row 2 column 2");
			}

			CopyString(archiveName, sizeof archiveName, token);
			tokenHandler = &CSV_SexyAssetParser::OnArchiveTypeSource;
		}

		void OnArchiveTypeSource(int row, int column, cstr token)
		{
			if (*token == 0 || row != 2 || column != 3)
			{
				Throw(0, "Expecting <archive-source> at row 2 column 3");
			}

			CopyString(archiveSource, sizeof archiveSource, token);
			tokenHandler = &CSV_SexyAssetParser::OnMemberDef;

			defRow = 3;
			defColumn = 1;
		}

		void OnSignature(int row, int column, cstr token)
		{
			if (!Eq(token, "AssetBuilder_CSV") || row != 1 || column != 1)
			{
				Throw(0, "Expecting AssetBuilder_CSV at row 1 column 1");
			}

			tokenHandler = &CSV_SexyAssetParser::OnArchiveType;
		}

		void OnToken(int row, int column, cstr token) override
		{
			(this->*tokenHandler)(row, column, token);
		}

		void Free() override
		{
			delete this;
		}

		void Reset() override
		{
			tokenHandler = &CSV_SexyAssetParser::OnSignature;
			currentColumn = 1;
			memberIndex.clear();
		}
	};

	struct TabbedCSVStreamTokenizer : ITabbedCSVTokenizer
	{
		std::vector<char> token;

		TabbedCSVStreamTokenizer()
		{

		}

		void AppendChar(char c)
		{
			token.push_back(c);
		}

		void Tokenize(cstr csvString, ICSVTokenParser& tokenParser) override
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

			token.clear();

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
					}
					else if (c == '\n')
					{
						column = 1;
						cursor.x = 1;
						row++;
						cursor.y++;
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
						AppendChar(c);
					}
					break;
				case STATE_READING_SIMPLE_TOKEN:
					if (c == 0)
					{
						goto end;
					}
					else if (c == '\t')
					{
						AppendChar(0);
						tokenParser.OnToken(row, column, token.data());
						token.clear();

						cursor.x++;
						column++;

						state = STATE_EXPECTING_TOKEN;
					}
					else if (c == '\n')
					{
						AppendChar(0);
						tokenParser.OnToken(row, column, token.data());
						token.clear();

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
						AppendChar(c);
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
						AppendChar(0);
						tokenParser.OnToken(row, column, token.data());
						token.clear();

						cursor.x++;
						column++;

						state = STATE_TERMINATING_STRING_LITERAL;
					}
					else if (c == '\n')
					{
						AppendChar('\n');
						cursor.x = 1;
						cursor.y++;
					}
					else if (c <= 32)
					{
						AppendChar(c);
						cursor.x++;
					}
					else if (c == '\\')
					{
						state = STATE_READING_STRING_LITERAL_ESCAPE;
						cursor.x++;
					}
					else
					{
						AppendChar(c);
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
						AppendChar('"');
						state = STATE_READING_STRING_LITERAL;
						cursor.x++;
					}
					else if (c == '\\')
					{
						AppendChar('\\');
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
						cursor.x = 1;
						cursor.y++;
						column = 1;
						row++;
						state = STATE_EXPECTING_TOKEN;
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

		void Free() override
		{
			delete this;
		}
	};
}

namespace Rococo::IO
{
	// In a tabbed CSV stream we have columns, delimited by the TAB character
	ITabbedCSVTokenizer* CreateTabbedCSVTokenizer()
	{
		return new TabbedCSVStreamTokenizer();
	}

	ICSVTokenParser* CreateSXYAParser(IMemberBuilder& memberBuilder)
	{
		return new CSV_SexyAssetParser(memberBuilder);
	}
}