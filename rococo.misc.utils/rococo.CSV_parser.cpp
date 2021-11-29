#include <rococo.api.h>
#include <rococo.io.h>
#include <rococo.strings.h>
#include <vector>

#include <rococo.csv.h>

#include <sexy.types.h>
#include <sexy.compiler.public.h>

using namespace Rococo;
using namespace Rococo::IO;

namespace
{
	using namespace Rococo::IO;
	using namespace Rococo::Sexy;

	struct CSV_SexyAssetParser : ICSVTokenParser
	{
		typedef void (CSV_SexyAssetParser::*FN_STATE)(int row, int column, cstr token, int32 stringLength);

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

		void OnMemberValueBool(int row, int column, cstr token, int32 stringLength)
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

		void OnMemberValueInt32(int row, int column, cstr token, int32 stringLength)
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

		void OnMemberValueInt64(int row, int column, cstr token, int32 stringLength)
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

		static float BinaryToFloat(uint32 binaryRepresentation)
		{
			return *(float*)(&binaryRepresentation);
		}

		void OnMemberValueFloat(int row, int column, cstr token, int32 stringLength)
		{
			if (defRow != row || defColumn != column)
			{
				Throw(0, "Expecting float hex value at row %d column %d", defRow, defColumn);
			}

			uint32 hexValue = 0;
			sscanf_s(token, "%x", &hexValue);

			float floatValue = BinaryToFloat(hexValue);

			memberBuilder.AddFloatMember(memberNameBuffer, floatValue);

			defRow++;
			defColumn -= 2;

			tokenHandler = &CSV_SexyAssetParser::OnMemberDef;
		}

		static double BinaryToDouble(uint64 binaryRepresentation)
		{
			return *(double*)(&binaryRepresentation);
		}

		void OnMemberValueDouble(int row, int column, cstr token, int32 stringLength)
		{
			if (defRow != row || defColumn != column)
			{
				Throw(0, "Expecting double hex value at row %d column %d", defRow, defColumn);
			}

			uint64 hexValue = 0;
			sscanf_s(token, "%llx", &hexValue);

			double doubleValue = BinaryToDouble(hexValue);

			memberBuilder.AddDoubleMember(memberNameBuffer, doubleValue);

			defRow++;
			defColumn -= 2;

			tokenHandler = &CSV_SexyAssetParser::OnMemberDef;
		}

		void OnMemberDerivativeSource(int row, int column, cstr token, int32 stringLength)
		{
			if (defRow != row || defColumn != column)
			{
				Throw(0, "Expecting <source-file> at row %d column %d", defRow, defColumn);
			}

			memberBuilder.AddDerivativeMember(memberTypeBuffer, memberNameBuffer, token);

			defRow++;
			defColumn -= 1; // Go back 2 then forward one, as sub-members are a column advance over their parents

			tokenHandler = &CSV_SexyAssetParser::OnMemberDef;
		}

		char interfaceDefSourceNameBuffer[Rococo::IO::MAX_PATHLEN];
		char instanceType[Rococo::MAX_FQ_NAME_LEN + 1];
		char instanceSource[Rococo::IO::MAX_PATHLEN];
		std::vector<char> stringConstantBuffer;

		void OnStringConstant(int row, int column, cstr token, int32 stringLength)
		{
			stringConstantBuffer.clear();
			stringConstantBuffer.resize(stringLength + 1);
			memcpy_s(stringConstantBuffer.data(), stringConstantBuffer.size(), token, stringLength);
			stringConstantBuffer[stringLength] = 0;

			defColumn++;

			tokenHandler = &CSV_SexyAssetParser::OnStringConstantLength;
		}

		void OnFastStringBuilderCapacity(int row, int column, cstr token, int32 stringLength)
		{
			if (defRow != row || defColumn != column)
			{
				Throw(0, "Expecting <string-constant> at row %d column %d", defRow, defColumn);
			}

			fstring text{ stringConstantBuffer.data(), (int32)stringConstantBuffer.size()-1 };

			int32 capacity = atoi(token);
			if (capacity < text.length)
			{
				Throw(0, "Expecting <string-buffer-capacity=%d> at row %d column %d to be no greater than the length of the string constant: %d characters", capacity, defRow, defColumn, text.length);
			}

			tokenHandler = &CSV_SexyAssetParser::OnMemberDef;

			memberBuilder.AddFastStringBuilder(memberNameBuffer, text, capacity, refName);

			defColumn -= 9;
			defRow++;
		}

		void OnFastStringBuilderLength(int row, int column, cstr token, int32 stringLength)
		{
			if (defRow != row || defColumn != column)
			{
				Throw(0, "Expecting <string-constant> at row %d column %d", defRow, defColumn);
			}

			int32 expectedLength = atoi(token);
			if (expectedLength + 1 != (int32) stringConstantBuffer.size())
			{
				Throw(0, "Expecting <string-constant> at row %d column %d to be of %d characters in length", defRow, defColumn-1, expectedLength);
			}

			tokenHandler = &CSV_SexyAssetParser::OnFastStringBuilderCapacity;
			defColumn++;
		}

		void OnFastStringBuilderName(int row, int column, cstr token, int32 stringLength)
		{
			if (defRow != row || defColumn != column)
			{
				Throw(0, "Expecting <string-constant> at row %d column %d", defRow, defColumn);
			}

			stringConstantBuffer.resize((size_t) (stringLength + 1));
			memcpy_s(stringConstantBuffer.data(), stringConstantBuffer.size(), token, stringLength);
			stringConstantBuffer[stringLength] = 0;

			tokenHandler = &CSV_SexyAssetParser::OnFastStringBuilderLength;
			defColumn++;
		}

		char refName[64];

		void OnInstanceRefName(int row, int column, cstr token, int32 stringLength)
		{
			if (defRow != row || defColumn != column)
			{
				Throw(0, "Expecting <instance-ref> at row %d column %d", defRow, defColumn);
			}

			SecureFormat(refName, sizeof refName, "%s", token);

			if (Eq(instanceType, "FastStringBuilder") && Eq(instanceSource, "Sys.Type.Strings.sxy"))
			{
				// Special case, FastStringBuilder has string / length / capacity elements appended to the line following the reference name

				tokenHandler = &CSV_SexyAssetParser::OnFastStringBuilderName;
				defColumn++;
			}
			else
			{
				memberBuilder.AddInterfaceMember(memberNameBuffer, memberTypeBuffer, interfaceDefSourceNameBuffer, instanceType, instanceSource, token);

				defColumn -= 6;
				++defRow;

				tokenHandler = &CSV_SexyAssetParser::OnMemberDef;
			}
		}

		void OnInstanceSource(int row, int column, cstr token, int32 stringLength)
		{
			if (defRow != row || defColumn != column)
			{
				Throw(0, "Expecting <instance-source> at row %d column %d", defRow, defColumn);
			}

			CopyString(instanceSource, sizeof instanceSource, token);

			defColumn++;

			tokenHandler = &CSV_SexyAssetParser::OnInstanceRefName;
		}

		void OnInstanceType(int row, int column, cstr token, int32 stringLength)
		{
			if (defRow != row || defColumn != column)
			{
				Throw(0, "Expecting <instance-type> at row %d column %d", defRow, defColumn);
			}

			CopyString(instanceType, sizeof instanceType, token);

			defColumn++;

			tokenHandler = &CSV_SexyAssetParser::OnInstanceSource;
		}

		void OnMemberInterfaceSource(int row, int column, cstr source, int32 stringLength)
		{
			if (defRow != row || defColumn != column)
			{
				Throw(0, "Expecting <source-file> at row %d column %d", defRow, defColumn);
			}

			CopyString(interfaceDefSourceNameBuffer, sizeof interfaceDefSourceNameBuffer, source);

			defColumn++;

			tokenHandler = &CSV_SexyAssetParser::OnInstanceType;
		}

		void OnMemberInterfaceType(int row, int column, cstr token, int32 stringLength)
		{
			if (defRow != row || defColumn != column)
			{
				Throw(0, "Expecting <source-file> at row %d column %d", defRow, defColumn);
			}

			CopyString(memberTypeBuffer, sizeof memberTypeBuffer, token);

			defColumn++;

			tokenHandler = &CSV_SexyAssetParser::OnMemberInterfaceSource;
		}

		void OnStringConstantLength(int row, int column, cstr token, int32 stringLength)
		{
			if (defRow != row || defColumn != column)
			{
				Throw(0, "Expecting <source-file> at row %d column %d", defRow, defColumn);
			}

			int stringConstantLength = atoi(token);
			if (stringConstantLength + 1 != (int32) stringConstantBuffer.size())
			{
				Throw(0, "Expecting string constant buffer length to be %d, but it was %llu", (stringConstantLength + 1), stringConstantBuffer.size());
			}

			memberBuilder.AddStringConstant(memberNameBuffer, stringConstantBuffer.data(), stringConstantLength);

			defColumn -= 3;
			defRow++;

			tokenHandler = &CSV_SexyAssetParser::OnMemberDef;
		}

		void OnMemberName(int row, int column, cstr token, int32 stringLength)
		{
			if (defRow != row)
			{
				Throw(0, "Expecting member name at row %d column %d", defRow, defColumn);
			}

			CopyString(memberNameBuffer, sizeof memberNameBuffer, token);

			defColumn++;

			if (Eq(memberTypeBuffer, "d"))
			{
				primitiveLine = true;
				tokenHandler = &CSV_SexyAssetParser::OnMemberValueDouble;
			}
			else if (Eq(memberTypeBuffer, "f"))
			{
				primitiveLine = true;
				tokenHandler = &CSV_SexyAssetParser::OnMemberValueFloat;
			}
			else if (Eq(memberTypeBuffer, "i"))
			{
				primitiveLine = true;
				tokenHandler = &CSV_SexyAssetParser::OnMemberValueInt32;
			}
			else if (Eq(memberTypeBuffer, "l"))
			{
				primitiveLine = true;
				tokenHandler = &CSV_SexyAssetParser::OnMemberValueInt64;
			}
			else if (Eq(memberTypeBuffer, "?"))
			{
				primitiveLine = true;
				tokenHandler = &CSV_SexyAssetParser::OnMemberValueBool;
			}
			else if (Eq(memberTypeBuffer, "@"))
			{
				primitiveLine = true;
				tokenHandler = &CSV_SexyAssetParser::OnMemberInterfaceType;
			}
			else if (Eq(memberTypeBuffer, "$"))
			{
				primitiveLine = true;
				tokenHandler = &CSV_SexyAssetParser::OnStringConstant;
			}
			else
			{
				primitiveLine = false;
				// Value was not primitive
				tokenHandler = &CSV_SexyAssetParser::OnMemberDerivativeSource;
			}
		}

		void OnMemberDef(int row, int column, cstr token, int32 stringLength)
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

		void OnArchiveType(int row, int column, cstr token, int32 stringLength)
		{
			if (*token == 0 || row != 2 || column != 1)
			{
				Throw(0, "Expecting <archive-type> at row 2 column 1");
			}

			CopyString(archiveType, sizeof archiveType, token);
			tokenHandler = &CSV_SexyAssetParser::OnArchiveName;
		}

		void OnArchiveName(int row, int column, cstr token, int32 stringLength)
		{
			if (*token == 0 || row != 2 || column != 2)
			{
				Throw(0, "Expecting <archive-name> at row 2 column 2");
			}

			CopyString(archiveName, sizeof archiveName, token);
			tokenHandler = &CSV_SexyAssetParser::OnArchiveTypeSource;
		}

		void OnArchiveTypeSource(int row, int column, cstr token, int32 stringLength)
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

		void OnSignature(int row, int column, cstr token, int32 stringLength)
		{
			if (!Eq(token, "AssetBuilder_CSV") || row != 1 || column != 1)
			{
				Throw(0, "Expecting AssetBuilder_CSV at row 1 column 1");
			}

			tokenHandler = &CSV_SexyAssetParser::OnArchiveType;
		}

		void OnToken(int row, int column, cstr token, int32 stringLengthPlusNul) override
		{
			try
			{
				(this->*tokenHandler)(row, column, token, stringLengthPlusNul - 1);
			}
			catch (IException& e)
			{
				Throw(e.ErrorCode(), "Error parsing token %s at row %d column %d\n%s", token, row, column, e.Message());
			}			
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
						tokenParser.OnToken(row, column, token.data(), (int32) token.size());
						token.clear();

						cursor.x++;
						column++;

						state = STATE_EXPECTING_TOKEN;
					}
					else if (c == '\n')
					{
						AppendChar(0);
						tokenParser.OnToken(row, column, token.data(), (int32) token.size());
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
						tokenParser.OnToken(row, column, token.data(), (int32) token.size());
						token.clear();

						cursor.x++;

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