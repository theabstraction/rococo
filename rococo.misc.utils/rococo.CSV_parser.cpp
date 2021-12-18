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
		typedef void (CSV_SexyAssetParser::* FN_STATE)(int row, int column, cstr token, int32 stringLength);

		std::vector<int> memberIndex;
		IMemberBuilder& memberBuilder;

		FN_STATE tokenHandler = &CSV_SexyAssetParser::OnSignature;
		int currentColumn = 1;

		char archiveType[Rococo::MAX_FQ_NAME_LEN + 1]; // The local type. For example, if a Sys.Maths.Vec2 were archived, the type would be Vec2f
		char archiveName[Rococo::MAX_FQ_NAME_LEN + 1]; // The variable name. Very often 'this'. Limited to legal Sexy variable names.
		char archiveSource[Rococo::IO::MAX_PATHLEN]; // The source file wherein the local type is defined. For example Vec2 is Sys.Maths.sxy

		CSV_SexyAssetParser(IMemberBuilder& _memberBuilder) :
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

		static float BinaryToFloat(uint32 binaryRepresentation)
		{
			return *(float*)(&binaryRepresentation);
		}

		static double BinaryToDouble(uint64 binaryRepresentation)
		{
			return *(double*)(&binaryRepresentation);
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

			fstring text{ stringConstantBuffer.data(), (int32)stringConstantBuffer.size() - 1 };

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
			if (expectedLength + 1 != (int32)stringConstantBuffer.size())
			{
				Throw(0, "Expecting <string-constant> at row %d column %d to be of %d characters in length", defRow, defColumn - 1, expectedLength);
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

			stringConstantBuffer.resize((size_t)(stringLength + 1));
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
		//		memberBuilder.AddInterfaceMember(memberNameBuffer, memberTypeBuffer, interfaceDefSourceNameBuffer, instanceType, instanceSource, token);

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
			if (stringConstantLength + 1 != (int32)stringConstantBuffer.size())
			{
				Throw(0, "Expecting string constant buffer length to be %d, but it was %llu", (stringConstantLength + 1), stringConstantBuffer.size());
			}

			memberBuilder.AddStringConstant(memberNameBuffer, stringConstantBuffer.data(), stringConstantLength);

			defColumn -= 3;
			defRow++;

			tokenHandler = &CSV_SexyAssetParser::OnMemberDef;
		}

		void OnArrayRefName(int row, int column, cstr token, int32 stringLength)
		{
			if (defRow != row || defColumn != column)
			{
				Throw(0, "Expecting <array-ref-name> at row %d column %d", defRow, defColumn);
			}

			//memberBuilder.AddArrayRefMember(memberNameBuffer, token);

			defColumn -= 2;
			defRow++;

			tokenHandler = &CSV_SexyAssetParser::OnMemberDef;
		}

		char memberTypeSource[Rococo::IO::MAX_PATHLEN];

		void OnObjectSource(int row, int column, cstr token, int32 stringLength)
		{
			if (defRow != row || defColumn != column)
			{
				Throw(0, "Expecting name at row %d column %d", defRow, defColumn);
			}

			memberBuilder.BuildObject(memberNameBuffer, memberTypeBuffer, token);

			defColumn = 1;
			defRow++;

			tokenHandler = &CSV_SexyAssetParser::OnMemberDef;
		}

		struct ArrayBuilder
		{
			int32 arrayCapacity;
			int32 arrayIndex;
			char elementTypeMemberName[Rococo::NAMESPACE_MAX_LENGTH];
			char elementTypeMemberType[Rococo::NAMESPACE_MAX_LENGTH];
		} array;

		std::vector<HString> memberTypes;
		char principalTypeName[Rococo::NAMESPACE_MAX_LENGTH] = { 0 };
		int32 activeMemberIndex = 0;

		void BuildValue(cstr token)
		{
			if (memberTypes.empty())
			{
				if (activeMemberIndex > 0)
				{
					Throw(0, "Bad active member index");
				}

				// There are no element types, which means the array->ElementType is a primitive or interface type
				if (Eq(principalTypeName, "Float32"))
				{
					memberBuilder.AddF32ItemValue(activeMemberIndex, (float)atof(token));
				}
				else if (Eq(principalTypeName, "Float64"))
				{
					memberBuilder.AddF64ItemValue(activeMemberIndex, atof(token));
				}
				else if (Eq(principalTypeName, "Int32"))
				{
					memberBuilder.AddI32ItemValue(activeMemberIndex, atoi(token));
				}
				else if (Eq(principalTypeName, "Int64"))
				{
					memberBuilder.AddI64ItemValue(activeMemberIndex, atoll(token));
				}
				else if (Eq(principalTypeName, "Boolean32"))
				{
					memberBuilder.AddBoolItemValue(activeMemberIndex, Eq(token, "Y"));
				}
				else if (*token == '#')
				{
					// Object Reference
					memberBuilder.AddObjectRefValue(activeMemberIndex, token);
				}
				else
				{
					Throw(0, "Unhandled type");
				}
			}
			else
			{
				if (activeMemberIndex > memberTypes.size())
				{
					Throw(0, "Bad active member index");
				}

				cstr typeName = memberTypes[activeMemberIndex];
				if (Eq(typeName, "f"))
				{
					memberBuilder.AddF32ItemValue(activeMemberIndex, (float)atof(token));
				}
				else if (Eq(typeName, "d"))
				{
					memberBuilder.AddF64ItemValue(activeMemberIndex, atof(token));
				}
				else if (Eq(typeName, "i"))
				{
					memberBuilder.AddI32ItemValue(activeMemberIndex, atoi(token));
				}
				else if (Eq(typeName, "l"))
				{
					memberBuilder.AddI64ItemValue(activeMemberIndex, atoll(token));
				}
				else if (Eq(typeName, "?"))
				{
					memberBuilder.AddBoolItemValue(activeMemberIndex, Eq(token, "Y"));
				}
				else if (Eq(typeName, "[]"))
				{
					memberBuilder.AddArrayRefValue(activeMemberIndex, token);
				}
				else
				{
					Throw(0, "Unhandled type");
				}

				activeMemberIndex++;
			}
		}

		void OnArrayValue(int row, int column, cstr token, int32 stringLength)
		{
			if (defRow != row)
			{
				Throw(0, "Expecting element value at row %d", defRow);
			}

			defRow++;

			if (*token == '[')
			{
				return;
			}

			BuildValue(token);

			if (activeMemberIndex >= memberTypes.size())
			{
				activeMemberIndex = 0;

				array.arrayIndex++;

				if (array.arrayIndex >= arrayLength)
				{
					tokenHandler = &CSV_SexyAssetParser::OnObjectName;
				}
				else
				{
					memberBuilder.SetArrayWriteIndex(array.arrayIndex);
				}
			}

			tokenHandler = &CSV_SexyAssetParser::OnArrayValue;
		}

		void OnArrayIndex(int row, int column, cstr token, int32 stringLength)
		{
			// [<index>]
			// N.B according to docs atoi will ignore the trailing ]
			array.arrayIndex = atoi(token + 1);

			defRow++;
			defColumn = 1;

			activeMemberIndex = 0;

			if (array.arrayIndex == arrayLength)
			{
				tokenHandler = &CSV_SexyAssetParser::OnObjectName;
			}
			else
			{
				tokenHandler = &CSV_SexyAssetParser::OnArrayValue;
				memberBuilder.SetArrayWriteIndex(array.arrayIndex);
			}
		}

		void OnArrayElementTypeMemberSource(int row, int column, cstr token, int32 stringLength)
		{
			if (defRow != row || defColumn != column)
			{
				Throw(0, "Expecting array element member type source at row %d column %d", defRow, defColumn);
			}

			memberBuilder.AddContainerItemDerivative(column - 3, array.elementTypeMemberName, array.elementTypeMemberType, token);

			defRow++;
			defColumn = 1;

			//array.elementMemberIndex++;
			//array.numberOfElementMembers++;

			tokenHandler = &CSV_SexyAssetParser::OnArrayElementTypeMemberName;
		}

		void OnElementTypeMemberType(int row, int column, cstr token, int32 stringLength)
		{
			OnMemberType(row, column, token, stringLength);
		}

		void OnMemberValue(int row, int column, cstr token, int32 stringLength)
		{
			if (defRow != row || column != 1)
			{
				Throw(0, "Expecting element value at row %d and column 1", defRow);
			}

			defRow++;

			BuildValue(token);

			if (activeMemberIndex >= memberTypes.size())
			{
				tokenHandler = &CSV_SexyAssetParser::OnObjectName;
			}
		}

		void OnMemberType(int row, int column, cstr token, int32 stringLength)
		{
			if (defRow != row || defColumn != column)
			{
				Throw(0, "Expecting member type of array element type at row %d column %d", defRow, defColumn);
			}

			int32 index = (int32) memberTypes.size();

			if (Eq(token, "f"))
			{
				// Float32 - f
				memberTypes.push_back(token);
				memberBuilder.AddTypeF32(index, column - 2, memberNameBuffer);
			}
			else if (Eq(token, "d"))
			{
				// Float64 - d
				memberTypes.push_back(token);
				memberBuilder.AddTypeF64(index, column - 2, memberNameBuffer);
			}
			else if (Eq(token, "i"))
			{
				// Int32 - i
				memberTypes.push_back(token);
				memberBuilder.AddTypeI32(index, column - 2, memberNameBuffer);
			}
			else if (Eq(token, "l"))
			{
				// Int64 - l
				memberTypes.push_back(token);
				memberBuilder.AddTypeI64(index, column - 2, memberNameBuffer);
			}
			else if (Eq(token, "?"))
			{
				// boolean - ?
				memberTypes.push_back(token);
				memberBuilder.AddTypeBool(index, column - 2, memberNameBuffer);
			}
			else if (Eq(token, "[]"))
			{
				memberTypes.push_back(token);
				memberBuilder.AddTypeArrayRef(index, column - 2, memberNameBuffer);
			}
			else
			{
				CopyString(memberTypeBuffer, sizeof memberTypeBuffer, token);
				defColumn++;
				tokenHandler = &CSV_SexyAssetParser::OnMemberDerivativeSource;
				return;
			}

			defRow++;
			defColumn--;

			tokenHandler = &CSV_SexyAssetParser::OnMemberDef;
		}

		void OnArrayElementTypeMemberName(int row, int column, cstr token, int32 stringLength)
		{
			if (defRow != row)
			{
				Throw(0, "Expecting element type member name at row %d", defRow);
			}

			if (*token == '[')
			{
				// Human readable index hint
				activeMemberIndex = 0;
				OnArrayIndex(row, column, token, stringLength);
				return;
			}
			else if (*token == '(')
			{
				// Enter member of derived type
				memberBuilder.EnterDerivedContainerItem();
				defRow++;
				return;
			}
			else if (*token == ')')
			{
				// Return to sibling of parent members
				memberBuilder.LeaveDerivedContainerItem();
				defRow++;
				return;
			}

			CopyString(array.elementTypeMemberName, sizeof array.elementTypeMemberName, token);

			defColumn = column + 1;

			tokenHandler = &CSV_SexyAssetParser::OnElementTypeMemberType;
		}

		void OnArrayCapacity(int row, int column, cstr token, int32 stringLength)
		{
			if (defRow != row || defColumn != column)
			{
				Throw(0, "Expecting array capacity at row %d column %d", defRow, defColumn);
			}

			array.arrayCapacity = atoi(token);

			if (array.arrayCapacity == 0)
			{
				Throw(0, "Zero length array capacity not permitted (row %d column %d)", defRow, defColumn);
			}

			if (array.arrayCapacity < arrayLength)
			{
				Throw(0, "Array capacity %d smaller than array length %d (row %d column %d)", array.arrayCapacity, arrayLength, defRow, defColumn);
			}

			defColumn = 1;
			defRow++;

			memberBuilder.AddArrayDefinition(memberNameBuffer, memberTypeBuffer, memberTypeSource, arrayLength, array.arrayCapacity);

			activeMemberIndex = 0;

			if (arrayLength > 0)
			{
				tokenHandler = &CSV_SexyAssetParser::OnArrayElementTypeMemberName;
			}
			else
			{
				tokenHandler = &CSV_SexyAssetParser::OnArrayRefName;
			}

			memberTypes.clear();
		}

		int arrayLength;

		void OnArrayLength(int row, int column, cstr token, int32 stringLength)
		{
			if (defRow != row || defColumn != column)
			{
				Throw(0, "Expecting array length at row %d column %d", defRow, defColumn);
			}

			arrayLength = atoi(token);

			defColumn++;

			tokenHandler = &CSV_SexyAssetParser::OnArrayCapacity;
		}

		void OnArrayTypeSource(int row, int column, cstr token, int32 stringLength)
		{
			if (defRow != row || defColumn != column)
			{
				Throw(0, "Expecting array element type source at row %d column %d", defRow, defColumn);
			}

			CopyString(memberTypeSource, sizeof memberTypeSource, token);

			defColumn++;

			tokenHandler = &CSV_SexyAssetParser::OnArrayLength;
		}

		void OnArrayType(int row, int column, cstr token, int32 stringLength)
		{
			if (defRow != row || defColumn != column)
			{
				Throw(0, "Expecting array element type at row %d column %d", defRow, defColumn);
			}

			CopyString(memberTypeBuffer, sizeof memberTypeBuffer, token);

			defColumn++;

			tokenHandler = &CSV_SexyAssetParser::OnArrayTypeSource;
		}

		void OnObjectType(int row, int column, cstr token, int32 stringLength)
		{
			if (defRow != row || defColumn != column)
			{
				Throw(0, "Expecting type at row %d column %d", defRow, defColumn);
			}

			defColumn++;

			CopyString(memberTypeBuffer, sizeof memberTypeBuffer, token);

			if (StartsWith(memberNameBuffer, "array"))
			{
				tokenHandler = &CSV_SexyAssetParser::OnArrayType;
			}
			else
			{
				CopyString(memberTypeBuffer, sizeof memberTypeBuffer, token);
				tokenHandler = &CSV_SexyAssetParser::OnObjectSource;
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
				Throw(0, "Expecting name at row %d column %d", defRow, defColumn);
			}

			if (column == 1 && *token == '#')
			{
				defRow++;
				activeMemberIndex = 0;
				tokenHandler = &CSV_SexyAssetParser::OnMemberValue;
			}
			else
			{
				CopyString(memberNameBuffer, sizeof memberNameBuffer, token);
				defColumn++;
				tokenHandler = &CSV_SexyAssetParser::OnMemberType;
			}
		}

		void OnObjectName(int row, int column, cstr token, int32 stringLength)
		{
			if (defRow != row || defColumn != column)
			{
				Throw(0, "Expecting object name at row %d column %d", defRow, defColumn);
			}

			CopyString(memberNameBuffer, sizeof memberNameBuffer, token);

			defColumn++;

			tokenHandler = &CSV_SexyAssetParser::OnObjectType;
		}

		void OnBlankLine(Vec2i cursorPosition) override
		{
			if (tokenHandler == &CSV_SexyAssetParser::OnMemberDef)
			{
				while (defColumn > 1)
				{
					// We are done building the previously defined object
					defColumn--;

					memberBuilder.ReturnToParent();
				}

				defColumn = 1;
				defRow++;

				tokenHandler = &CSV_SexyAssetParser::OnObjectName;
			}
			else if (tokenHandler == &CSV_SexyAssetParser::OnObjectName)
			{
				defColumn = 1;
				defRow++;
			}
			else
			{
				Throw(0, "Unexpected blank line at (%d,%d)", cursorPosition.x, cursorPosition.y);
			}
		}

		void OnSignature(int row, int column, cstr token, int32 stringLength)
		{
			if (!Eq(token, "AssetBuilder_TabbedCSV_1.0") || row != 1 || column != 1)
			{
				Throw(0, "Expecting AssetBuilder_TabbedCSV_1.0 at row 1 column 1");
			}

			defRow = 2;
			defColumn = 1;

			tokenHandler = &CSV_SexyAssetParser::OnObjectName;
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