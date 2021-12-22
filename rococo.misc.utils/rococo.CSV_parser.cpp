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

		char typeBuffer[Rococo::MAX_FQ_NAME_LEN + 1];
		char nameBuffer[Rococo::MAX_FQ_NAME_LEN + 1];

		void BuildDerivativeType(int row, int column, cstr token, int32 stringLength, cstr memberName)
		{
			if (defRow != row || defColumn != column)
			{
				Throw(0, "Expecting <source-file> at row %d column %d", defRow, defColumn);
			}

			memberBuilder.AddTypeDerivative(column - 2, typeBuffer, memberName, token);

			defRow++;
			defColumn -= 1; // Go back 2 then forward one, as sub-members are a column advance over their parents
		}

		void OnMemberDerivativeSource(int row, int column, cstr token, int32 stringLength)
		{
			BuildDerivativeType(row, column, token, stringLength, nameBuffer);
			tokenHandler = &CSV_SexyAssetParser::OnMemberDef;
		}

		void OnElementMemberDerivativeSource(int row, int column, cstr token, int32 stringLength)
		{
			BuildDerivativeType(row, column, token, stringLength, array.elementTypeMemberName);
			tokenHandler = &CSV_SexyAssetParser::OnElementMemberDef;
		}

		void OnMemberInterfaceSource(int row, int column, cstr token, int32 stringLength)
		{
			if (defRow != row || defColumn != column)
			{
				Throw(0, "Expecting <interface-source-file> at row %d column %d", defRow, defColumn);
			}

			memberTypes.push_back(VARTYPE_Derivative);
			memberBuilder.AddTypeInterface(column - 3, typeBuffer, nameBuffer, token);

			defRow++;
			defColumn -= 3;

			tokenHandler = &CSV_SexyAssetParser::OnMemberDef;
		}

		void OnElementMemberInterfaceSource(int row, int column, cstr token, int32 stringLength)
		{
			OnMemberInterfaceSource(row, column, token, stringLength);
			tokenHandler = &CSV_SexyAssetParser::OnElementMemberDef;
		}

		void OnMemberInterfaceName(int row, int column, cstr token, int32 stringLength)
		{
			if (defRow != row || defColumn != column)
			{
				Throw(0, "Expecting <interface-name> at row %d column %d", defRow, defColumn);
			}

			CopyString(typeBuffer, sizeof typeBuffer, token);
			
			defColumn++;

			tokenHandler = &CSV_SexyAssetParser::OnMemberInterfaceSource;
		}

		void OnElementMemberInterfaceName(int row, int column, cstr token, int32 stringLength)
		{
			OnMemberInterfaceName(row, column, token, stringLength);
			tokenHandler = &CSV_SexyAssetParser::OnElementMemberInterfaceSource;
		}

		char interfaceDefSourceNameBuffer[Rococo::IO::MAX_PATHLEN];
		char instanceType[Rococo::MAX_FQ_NAME_LEN + 1];
		char instanceSource[Rococo::IO::MAX_PATHLEN];
		
		int32 stringBuilderCapacity = 0;

		void OnFastStringBuilderLength(int row, int column, cstr token, int32 stringLength)
		{
			if (defRow != row || defColumn != column)
			{
				Throw(0, "Expecting <string-constant> at row %d column %d", defRow, defColumn);
			}

			stringConstantLength = atoi(token);
			tokenHandler = &CSV_SexyAssetParser::OnFastStringBuilderCapacity;
			defColumn++;
		}

		void OnFastStringBuilderCapacity(int row, int column, cstr token, int32 stringLength)
		{
			if (defRow != row || defColumn != column)
			{
				Throw(0, "Expecting <string-constant> at row %d column %d", defRow, defColumn);
			}

			stringBuilderCapacity = atoi(token);

			if (stringBuilderCapacity < 0)
			{
				Throw(0, "Expecting <stringBuilderCapacity> capacity must be >= 0");
			}

			tokenHandler = &CSV_SexyAssetParser::OnFastStringBuilderCharSequence;

			defColumn++;
		}

		void OnFastStringBuilderCharSequence(int row, int column, cstr token, int32 stringLength)
		{
			if (defRow != row || defColumn != column)
			{
				Throw(0, "Expecting <string-constant> at row %d column %d", defRow, defColumn);
			}

			if (stringLength != stringConstantLength)
			{
				Throw(0, "Expected string length to be %d at row %d column %d", stringConstantLength, defRow, defColumn);
			}

			memberBuilder.AddFastStringBuilder(nameBuffer, fstring{ token, stringLength }, stringBuilderCapacity);

			tokenHandler = &CSV_SexyAssetParser::OnObjectName;
			defColumn = 1;
			defRow++;
		}

		void OnMemberInterfaceType(int row, int column, cstr token, int32 stringLength)
		{
			if (defRow != row || defColumn != column)
			{
				Throw(0, "Expecting <source-file> at row %d column %d", defRow, defColumn);
			}

			CopyString(typeBuffer, sizeof typeBuffer, token);

			defColumn++;

			tokenHandler = &CSV_SexyAssetParser::OnMemberInterfaceName;
		}

		int stringConstantLength = 0;

		void OnStringConstantCharSequence(int row, int column, cstr token, int32 stringLength)
		{
			if (stringConstantLength != stringLength)
			{
				Throw(0, "Expecting string constant buffer length to be %d, but it was %d", (stringConstantLength + 1), stringLength);
			}

			memberBuilder.AddStringConstant(nameBuffer, token, stringLength);

			defColumn = 1;
			defRow++;

			tokenHandler = &CSV_SexyAssetParser::OnObjectName;
		}

		void OnStringConstantLength(int row, int column, cstr token, int32 stringLength)
		{
			if (defRow != row || defColumn != column)
			{
				Throw(0, "Expecting <source-file> at row %d column %d", defRow, defColumn);
			}

			stringConstantLength = atoi(token);

			defColumn++;

			tokenHandler = &CSV_SexyAssetParser::OnStringConstantCharSequence;
		}

		void OnArrayRefName(int row, int column, cstr token, int32 stringLength)
		{
			if (defRow != row || defColumn != column)
			{
				Throw(0, "Expecting <array-ref-name> at row %d column %d", defRow, defColumn);
			}

			//memberBuilder.AddArrayRefMember(nameBuffer, token);

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

			memberBuilder.BuildObject(nameBuffer, typeBuffer, token);

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

		std::vector<VARTYPE> memberTypes;
		int32 activeMemberIndex = 0;

		void BuildValueAndAdvanceMemberIndex(cstr token)
		{
			if (memberTypes.empty())
			{
				if (activeMemberIndex > 0)
				{
					Throw(0, "Bad active member index");
				}

				// There are no element types, which means the array->ElementType is a primitive or interface type
				if (Eq(typeBuffer, "Float32") && Eq(memberTypeSource, "Sys.Types.sxy"))
				{
					memberBuilder.AddF32ItemValue(activeMemberIndex, (float)atof(token));
				}
				else if (Eq(typeBuffer, "Float64") && Eq(memberTypeSource, "Sys.Types.sxy"))
				{
					memberBuilder.AddF64ItemValue(activeMemberIndex, atof(token));
				}
				else if (Eq(typeBuffer, "Int32") && Eq(memberTypeSource, "Sys.Types.sxy"))
				{
					memberBuilder.AddI32ItemValue(activeMemberIndex, atoi(token));
				}
				else if (Eq(typeBuffer, "Int64") && Eq(memberTypeSource, "Sys.Types.sxy"))
				{
					memberBuilder.AddI64ItemValue(activeMemberIndex, atoll(token));
				}
				else if (Eq(typeBuffer, "Boolean32") && Eq(memberTypeSource, "Sys.Types.sxy"))
				{
					memberBuilder.AddBoolItemValue(activeMemberIndex, Eq(token, "Y"));
				}
				else if (StartsWith(typeBuffer, "_Null_"))
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

				VARTYPE typeName = memberTypes[activeMemberIndex];
				switch (typeName)
				{
				case VARTYPE_Float32:
					memberBuilder.AddF32ItemValue(activeMemberIndex, (float)atof(token));
					break;
				case VARTYPE_Float64:
					memberBuilder.AddF64ItemValue(activeMemberIndex, atof(token));
					break;
				case VARTYPE_Int32:
					memberBuilder.AddI32ItemValue(activeMemberIndex, atoi(token));
					break;
				case VARTYPE_Int64:
					memberBuilder.AddI64ItemValue(activeMemberIndex, atoll(token));
					break;
				case VARTYPE_Bool:
					memberBuilder.AddBoolItemValue(activeMemberIndex, Eq(token, "Y"));
					break;
				case VARTYPE_Array:
					memberBuilder.AddArrayRefValue(activeMemberIndex, token);
					break;
				case VARTYPE_Derivative:
					memberBuilder.AddObjectRefValue(activeMemberIndex, token);
					break;
				default:
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

			BuildValueAndAdvanceMemberIndex(token);

			if (memberTypes.empty() || activeMemberIndex >= memberTypes.size())
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
					tokenHandler = &CSV_SexyAssetParser::OnArrayIndex;
				}
			}
		}

		void OnArrayIndex(int row, int column, cstr token, int32 stringLength)
		{
			// [<index>]
			// N.B according to docs atoi will ignore the trailing ]
			array.arrayIndex = atoi(token + 1);

			defRow++;
			defColumn = 1;

			activeMemberIndex = 0;

			if (array.arrayIndex >= arrayLength)
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

			tokenHandler = &CSV_SexyAssetParser::OnArrayElementTypeMemberName;
		}

		void OnElementMemberValue(int row, int column, cstr token, int32 stringLength)
		{
			if (defRow != row || column != 1)
			{
				Throw(0, "Expecting element value at row %d and column 1", defRow);
			}

			defRow++;

			BuildValueAndAdvanceMemberIndex(token);

			if (activeMemberIndex >= memberTypes.size())
			{
				activeMemberIndex = 0;
				array.arrayIndex++;

				if (array.arrayIndex < array.arrayCapacity)
				{
					tokenHandler = &CSV_SexyAssetParser::OnArrayIndex;
					//memberBuilder.SetArrayWriteIndex(array.arrayIndex);
				}
				else
				{
					tokenHandler = &CSV_SexyAssetParser::OnObjectName;
				}
			}
		}

		void OnMemberValue(int row, int column, cstr token, int32 stringLength)
		{
			if (defRow != row || column != 1)
			{
				Throw(0, "Expecting element value at row %d and column 1", defRow);
			}

			defRow++;

			BuildValueAndAdvanceMemberIndex(token);

			if (activeMemberIndex >= memberTypes.size())
			{
				tokenHandler = &CSV_SexyAssetParser::OnObjectName;
			}
		}

		bool TryBuildSimpleType(int32 memberDepth, cstr token)
		{
			if (Eq(token, "f"))
			{
				memberTypes.push_back(VARTYPE_Float32);
				memberBuilder.AddTypeF32(memberDepth, nameBuffer);
			}
			else if (Eq(token, "d"))
			{
				memberTypes.push_back(VARTYPE_Float64);
				memberBuilder.AddTypeF64(memberDepth, nameBuffer);
			}
			else if (Eq(token, "i"))
			{
				memberTypes.push_back(VARTYPE_Int32);
				memberBuilder.AddTypeI32(memberDepth, nameBuffer);
			}
			else if (Eq(token, "l"))
			{
				memberTypes.push_back(VARTYPE_Int64);
				memberBuilder.AddTypeI64(memberDepth, nameBuffer);
			}
			else if (Eq(token, "?"))
			{
				memberTypes.push_back(VARTYPE_Bool);
				memberBuilder.AddTypeBool(memberDepth, nameBuffer);
			}
			else if (Eq(token, "[]"))
			{
				memberTypes.push_back(VARTYPE_Array);
				memberBuilder.AddTypeArrayRef(memberDepth, nameBuffer);
			}
			else
			{
				return false;
			}

			return true;
		}

		void OnElementTypeMemberType(int row, int column, cstr token, int32 stringLength)
		{
			if (defRow != row || defColumn != column)
			{
				Throw(0, "Expecting member type of array element type at row %d column %d", defRow, defColumn);
			}

			if (TryBuildSimpleType(column - 2, token))
			{
				defRow++;
				defColumn--;
				tokenHandler = &CSV_SexyAssetParser::OnElementMemberDef;
			}
			else if (Eq(token, "@"))
			{
				CopyString(typeBuffer, sizeof typeBuffer, token);
				defColumn++;
				tokenHandler = &CSV_SexyAssetParser::OnElementMemberInterfaceName;
			}
			else
			{
				CopyString(typeBuffer, sizeof typeBuffer, token);
				defColumn++;
				tokenHandler = &CSV_SexyAssetParser::OnElementMemberDerivativeSource;
			}
		}

		void OnMemberType(int row, int column, cstr token, int32 stringLength)
		{
			if (defRow != row || defColumn != column)
			{
				Throw(0, "Expecting member type of array element type at row %d column %d", defRow, defColumn);
			}

			if (TryBuildSimpleType(column - 2, token))
			{
				defRow++;
				defColumn--;
				tokenHandler = &CSV_SexyAssetParser::OnMemberDef;
			}
			else if (Eq(token, "@"))
			{
				CopyString(typeBuffer, sizeof typeBuffer, token);
				defColumn++;
				tokenHandler = &CSV_SexyAssetParser::OnMemberInterfaceName;
			}
			else
			{
				CopyString(typeBuffer, sizeof typeBuffer, token);
				defColumn++;
				tokenHandler = &CSV_SexyAssetParser::OnMemberDerivativeSource;
			}
		}

		void OnArrayElementTypeMemberName(int row, int column, cstr token, int32 stringLength)
		{
			if (defRow != row)
			{
				Throw(0, "Expecting element type member name at row %d", defRow);
			}

			if (*token == '[')
			{
				// Note, if our array is of primitives, then the element type is given by the typeBuffer variable, and we expect an array index after the array def line
				
				// Human readable index hint
				activeMemberIndex = 0;
				OnArrayIndex(row, column, token, stringLength);
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

			memberBuilder.AddArrayDefinition(nameBuffer, typeBuffer, memberTypeSource, arrayLength, array.arrayCapacity);

			activeMemberIndex = 0;

			tokenHandler = &CSV_SexyAssetParser::OnArrayElementTypeMemberName;
		}

		int arrayLength;

		void OnArrayLength(int row, int column, cstr token, int32 stringLength)
		{
			if (defRow != row || defColumn != column)
			{
				Throw(0, "Expecting array length at row %d column %d", defRow, defColumn);
			}

			arrayLength = atoi(token);

			if (arrayLength < 0)
			{
				Throw(0, "Expecting positive array length at row %d column %d", defRow, defColumn);
			}

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

			CopyString(typeBuffer, sizeof typeBuffer, token);

			defColumn++;

			tokenHandler = &CSV_SexyAssetParser::OnArrayTypeSource;
		}

		void OnNullObjectSource(int row, int column, cstr token, int32 stringLength)
		{
			if (defRow != row || defColumn != column)
			{
				Throw(0, "Expecting <null object source> at row %d column %d", defRow, defColumn);
			}

			defColumn = 1;
			defRow++;

			memberBuilder.AddNullObject(nameBuffer, typeBuffer, token);

			tokenHandler = &CSV_SexyAssetParser::OnObjectName;
		}

		void OnObjectType(int row, int column, cstr token, int32 stringLength)
		{
			if (defRow != row || defColumn != column)
			{
				Throw(0, "Expecting type at row %d column %d", defRow, defColumn);
			}

			defColumn++;

			if (Eq(token, "_SC"))
			{
				// String constant
				tokenHandler = &CSV_SexyAssetParser::OnStringConstantLength;
				return;
			}
			else if (Eq(token, "_SB"))
			{
				// String builder
				tokenHandler = &CSV_SexyAssetParser::OnFastStringBuilderLength;
				return;
			}
			else if (StartsWith(token, "_Null_"))
			{
				CopyString(typeBuffer, sizeof typeBuffer, token);

				// Null object definition.
				tokenHandler = &CSV_SexyAssetParser::OnNullObjectSource;
				return;
			}
			else
			{
				CopyString(typeBuffer, sizeof typeBuffer, token);
				tokenHandler = &CSV_SexyAssetParser::OnObjectSource;
			}
		}

		void UpdateMemberDepth(int row, int column)
		{
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
		}

		void OnElementMemberDef(int row, int column, cstr token, int32 stringLength)
		{
			UpdateMemberDepth(row, column);

			if (column == 1 && *token == '[')
			{
				defRow++;
				activeMemberIndex = 0;
				tokenHandler = &CSV_SexyAssetParser::OnElementMemberValue;
			}
			else
			{
				CopyString(array.elementTypeMemberName, sizeof array.elementTypeMemberName, token);
				defColumn++;
				tokenHandler = &CSV_SexyAssetParser::OnElementTypeMemberType;
			}
		}

		void OnMemberDef(int row, int column, cstr token, int32 stringLength)
		{
			UpdateMemberDepth(row, column);

			if (column == 1 && *token == '#')
			{
				defRow++;
				activeMemberIndex = 0;
				tokenHandler = &CSV_SexyAssetParser::OnMemberValue;
			}
			else
			{
				CopyString(nameBuffer, sizeof nameBuffer, token);
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

			CopyString(nameBuffer, sizeof nameBuffer, token);

			defColumn++;

			memberTypes.clear();
			activeMemberIndex = 0;

			if (StartsWith(nameBuffer, "array"))
			{
				array.arrayIndex = 0;
				tokenHandler = &CSV_SexyAssetParser::OnArrayType;
			}
			else if (*nameBuffer == '#')
			{
				tokenHandler = &CSV_SexyAssetParser::OnObjectType;
			}
			else
			{
				Throw(0, "Expecting object name at row %d column %d (Either array<suffix> or #Object<suffix>)", defRow, defColumn);
			}
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
			memberIndex.clear();
		}
	};

	template<class PARSER> void Tokenize(cstr csvString, PARSER& tokenParser)
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

ROCOCOAPI ICSVLine
{
	virtual size_t TokenCount() const = 0;
	virtual cstr operator[](size_t index) const = 0;
};

ROCOCOAPI ICSVLineParser
{
	virtual void OnBadChar(Vec2i cursor, char c) = 0;
	virtual void OnLine(const ICSVLine& line) = 0;
};

namespace
{
	thread_local std::vector<char> csv_token;
}

namespace Rococo::IO
{
	void ParseTabbedCSVString(cstr csvString, ICSVTokenParser& tokenParser)
	{
		csv_token.clear();

		struct CELL_PARSER
		{
			ICSVTokenParser& tokenParser;
			cstr startRaw;

			void OnBlankLine(Vec2i cursor)
			{
				tokenParser.OnBlankLine(cursor);
			}

			void OnBadChar(Vec2i cursor, char c)
			{
				tokenParser.OnBadChar(cursor, c);
			}

			void AppendStringLiteral(char c)
			{
				csv_token.push_back(c);
			}

			void StartRaw(cstr p)
			{
				startRaw = p;
			}

			void EndRaw(cstr endRaw)
			{
				for (cstr i = startRaw; i < endRaw; ++i)
				{
					csv_token.push_back(*i);					
				}
			}

			void Submit(int row, int column)
			{
				if (!csv_token.empty())
				{
					AppendStringLiteral(0);
					tokenParser.OnToken(row, column, csv_token.data(), (int32)csv_token.size());
					csv_token.clear();
				}
			}
			
			void OnNewLine()
			{

			}

		} parser{ tokenParser };
		Tokenize<CELL_PARSER>(csvString, parser);
	}

	struct CSVToken
	{
		std::vector<char> s;
	};

	thread_local std::vector<CSVToken> csv_tokens;
	thread_local size_t tokenIndex = 0;

	void ClearStringLiterals()
	{
		for (auto& sl : csv_tokens)
		{
			sl.s.clear();
		}
	}

	void ParseTabbedCSVString(cstr csvString, ICSVLineParser& lineParser)
	{
		struct LINE_PARSER
		{
			ICSVLineParser& lineParser;

			void ResetBuilder()
			{
				ClearStringLiterals();
			}

			void OnBlankLine(Vec2i cursor)
			{
				ResetBuilder();
			}

			void OnBadChar(Vec2i cursor, char c)
			{
				lineParser.OnBadChar(cursor, c);
			}

			void AppendStringLiteral(char c)
			{
				startRaw = nullptr;

				if (tokenIndex >= csv_tokens.size())
				{
					csv_tokens.push_back(CSVToken());
				}

				csv_tokens[tokenIndex].s.push_back(c);
			}

			cstr startRaw = nullptr;
			cstr endRaw = nullptr;

			void StartRaw(cstr p)
			{
				startRaw = p;
			}

			void EndRaw(cstr p)
			{
				endRaw = p;
			}

			void Submit(int row, int column)
			{
				if (startRaw)
				{
					if (tokenIndex >= csv_tokens.size())
					{
						csv_tokens.push_back(CSVToken());
					}

					auto& s = csv_tokens[tokenIndex++].s;

					for (auto p = startRaw; p != endRaw; p++)
					{
						s.push_back(*p);		
					}

					s.push_back(0);

					startRaw = nullptr;
				}
			}

			void OnNewLine()
			{
				struct CSVLine : ICSVLine
				{
					size_t TokenCount() const override
					{
						return tokenIndex;
					}

					cstr operator[] (size_t index) const override
					{
						return csv_tokens[index].s.data();
					}
				} line;

				if (tokenIndex > 0) lineParser.OnLine(line);

				ResetBuilder();
			}
		} parser{ lineParser };
		Tokenize<LINE_PARSER>(csvString, parser);
		parser.OnNewLine();
	}

	ICSVTokenParser* CreateSXYAParser(IMemberBuilder& memberBuilder)
	{
		return new CSV_SexyAssetParser(memberBuilder);
	}
}