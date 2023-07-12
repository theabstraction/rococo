#include <rococo.api.h>
#include <rococo.io.h>
#include <rococo.strings.h>
#include <vector>

#include <rococo.csv.h>

#include <sexy.types.h>
#include <sexy.compiler.public.h>

#include <rococo.csv.inl>

using namespace Rococo;
using namespace Rococo::IO;
using namespace Rococo::Strings;

ROCOCO_INTERFACE ICSVLine
{
	virtual size_t TokenCount() const = 0;
	virtual fstring operator[](size_t index) const = 0;
	virtual int Row() const = 0;
};

ROCOCO_INTERFACE ICSVLineParser
{
	virtual void OnBadChar(Vec2i cursor, char c) = 0;
	virtual void OnLine(const ICSVLine& line) = 0;
};

static thread_local std::vector<VARTYPE> memberTypes;

struct CSV_Line_by_Line_SexyAssetParser: ICSVLineParser
{
	typedef void (CSV_Line_by_Line_SexyAssetParser::* FN_STATE)(const ICSVLine& line);
	FN_STATE state = &CSV_Line_by_Line_SexyAssetParser::OnSignature;
	IMemberBuilder& builder;

	int32 activeMemberIndex = 0;

	char objectType[Rococo::MAX_FQ_NAME_LEN + 1];
	char objectModule[Rococo::MAX_FQ_NAME_LEN + 1];

	CSV_Line_by_Line_SexyAssetParser(IMemberBuilder& _builder): builder(_builder)
	{

	}

	bool TryBuildSimpleType(int32 memberDepth, cstr token, cstr name)
	{
		if (Eq(token, "f"))
		{
			memberTypes.push_back(VARTYPE_Float32);
			builder.AddTypeF32(memberDepth, name);
		}
		else if (Eq(token, "d"))
		{
			memberTypes.push_back(VARTYPE_Float64);
			builder.AddTypeF64(memberDepth, name);
		}
		else if (Eq(token, "i"))
		{
			memberTypes.push_back(VARTYPE_Int32);
			builder.AddTypeI32(memberDepth, name);
		}
		else if (Eq(token, "l"))
		{
			memberTypes.push_back(VARTYPE_Int64);
			builder.AddTypeI64(memberDepth, name);
		}
		else if (Eq(token, "?"))
		{
			memberTypes.push_back(VARTYPE_Bool);
			builder.AddTypeBool(memberDepth, name);
		}
		else if (Eq(token, "[]"))
		{
			memberTypes.push_back(VARTYPE_Array);
			builder.AddTypeArrayRef(memberDepth, name);
		}
		else if (Eq(token, "->"))
		{
			memberTypes.push_back(VARTYPE_Map);
			builder.AddTypeMapRef(memberDepth, name);
		}
		else if (Eq(token, "..."))
		{
			memberTypes.push_back(VARTYPE_List);
			builder.AddTypeListRef(memberDepth, name);
		}
		else
		{
			return false;
		}

		return true;
	}

	void OnSignature(const ICSVLine& line)
	{
		if (line.TokenCount() == 1)
		{
			if (Eq(line[0], "AssetBuilder_TabbedCSV_1.0"))
			{
				state = &CSV_Line_by_Line_SexyAssetParser::OnObjectLine;
				return;
			}
		}

		Throw(0, "Expecting match to AssetBuilder_TabbedCSV_1.0 on first line of the file");
	}

	void BuildMember(const ICSVLine& line)
	{
		if (arrayIndex == 0 && line.TokenCount() == 1)
		{
			if (*line[0] == '[')
			{
				// An index
				state = &CSV_Line_by_Line_SexyAssetParser::OnArrayValue;
				return;
			}
		}

		int indent = 0;

		for (; indent < line.TokenCount(); indent++)
		{
			if (*line[indent] != 0)
			{
				break;
			}
		}

		size_t definiteTokens = line.TokenCount() - indent;

		if (definiteTokens == 2)
		{
			cstr memberName = line[indent];
			cstr memberTypeToken = line[indent + 1];

			if (!TryBuildSimpleType(indent, memberTypeToken, memberName))
			{
				Throw(0, "Could not parse '%s %s' as a name v type pair", memberName, memberTypeToken);
			}
		}
		else if (definiteTokens == 3)
		{
			cstr memberName = line[indent];
			cstr memberDerivedType = line[indent + 1];
			cstr memberDerivedModule = line[indent + 2];

			builder.AddContainerItemDerivative(indent, memberName, memberDerivedType, memberDerivedModule);
		}
		else if (definiteTokens == 4)
		{
			cstr memberName = line[indent];
			cstr refChar = line[indent + 1];
			cstr memberInterfaceType = line[indent + 2];
			cstr memberInterfaceModule = line[indent + 3];

			if (!Eq(refChar, "@"))
			{
				Throw(0, "Could not parse '%s %s %s %s' as '<member-name> @ <interface-type> <interface-module>'", memberName, refChar, memberInterfaceType, memberInterfaceModule);
			}

			memberTypes.push_back(VARTYPE_Derivative);
			builder.AddTypeInterface(indent, memberInterfaceType, memberName, memberInterfaceModule);
		}
		else
		{
			Throw(0, "Could not build member. Expecting 2, 3 or 4 tokens");
		}
	}

	void OnObjectMember(const ICSVLine& line)
	{
		if (line.TokenCount() == 1)
		{
			if (Eq(line[0], "#"))
			{

				activeMemberIndex = 0;
				state = &CSV_Line_by_Line_SexyAssetParser::OnObjectValue;
				return;
			}
		}

		BuildMember(line);
	}

	void BuildValueAndAdvanceMemberIndex(cstr token)
	{
		if (memberTypes.empty())
		{
			if (activeMemberIndex > 0)
			{
				Throw(0, "Bad active member index");
			}

			// There are no element types, which means the array->ElementType is a primitive or interface type
			if (Eq(objectType, "Float32") && Eq(objectModule, "!Intrinsics!"))
			{
				builder.AddF32ItemValue(activeMemberIndex, (float)atof(token));
			}
			else if (Eq(objectType, "Float64") && Eq(objectModule, "!Intrinsics!"))
			{
				builder.AddF64ItemValue(activeMemberIndex, atof(token));
			}
			else if (Eq(objectType, "Int32") && Eq(objectModule, "!Intrinsics!"))
			{
				builder.AddI32ItemValue(activeMemberIndex, atoi(token));
			}
			else if (Eq(objectType, "Int64") && Eq(objectModule, "!Intrinsics!"))
			{
				builder.AddI64ItemValue(activeMemberIndex, atoll(token));
			}
			else if (Eq(objectType, "Boolean32") && Eq(objectModule, "!Intrinsics!"))
			{
				builder.AddBoolItemValue(activeMemberIndex, Eq(token, "Y"));
			}
			else if (StartsWith(objectType, "_Null_"))
			{
				// Object Reference
				builder.AddObjectRefValue(activeMemberIndex, token);
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
				builder.AddF32ItemValue(activeMemberIndex, (float)atof(token));
				break;
			case VARTYPE_Float64:
				builder.AddF64ItemValue(activeMemberIndex, atof(token));
				break;
			case VARTYPE_Int32:
				builder.AddI32ItemValue(activeMemberIndex, atoi(token));
				break;
			case VARTYPE_Int64:
				builder.AddI64ItemValue(activeMemberIndex, atoll(token));
				break;
			case VARTYPE_Bool:
				builder.AddBoolItemValue(activeMemberIndex, Eq(token, "Y"));
				break;
			case VARTYPE_Array:
				builder.AddArrayRefValue(activeMemberIndex, token);
				break;
			case VARTYPE_Derivative:
				builder.AddObjectRefValue(activeMemberIndex, token);
				break;
			case VARTYPE_Map:
				builder.AddMapRefValue(activeMemberIndex, token);
				break;
			case VARTYPE_List:
				builder.AddListRefValue(activeMemberIndex, token);
				break;
			default:
				Throw(0, "Unhandled type");
			}

			activeMemberIndex++;
		}
	}

	void OnObjectValue(const ICSVLine& line)
	{
		if (line.TokenCount() != 1)
		{
			Throw(0, "Expecting one member value on the line, but the token count was %llu", line.TokenCount());
		}

		BuildValueAndAdvanceMemberIndex(line[0]);

		if (activeMemberIndex == memberTypes.size())
		{
			state = &CSV_Line_by_Line_SexyAssetParser::OnObjectLine;
		}
	}

	int arrayIndex = 0;
	int arrayLength = 0;
	int arrayCapacity = 0;

	void OnArrayIndex(const ICSVLine& line)
	{
		if (line.TokenCount() == 1)
		{
			if (*line[0] == '[')
			{
				state = &CSV_Line_by_Line_SexyAssetParser::OnArrayValue;
				return;
			}
		}

		Throw(0, "Expecting [array-index]");
	}

	void OnArrayValue(const ICSVLine& line)
	{
		if (line.TokenCount() != 1)
		{
			Throw(0, "Expecting one member value on the line, but the token count was %llu", line.TokenCount());
		}

		BuildValueAndAdvanceMemberIndex(line[0]);

		if (activeMemberIndex >= memberTypes.size())
		{
			activeMemberIndex = 0;
			arrayIndex++;
			if (arrayIndex >= arrayLength)
			{
				state = &CSV_Line_by_Line_SexyAssetParser::OnObjectLine;
			}
			else
			{
				builder.SetArrayWriteIndex(arrayIndex);
				state = &CSV_Line_by_Line_SexyAssetParser::OnArrayIndex;
			}
		}
	}

	void OnMapValue(const ICSVLine& line)
	{
		if (line.TokenCount() != 1)
		{
			Throw(0, "Expecting one member value on the line, but the token count was %llu", line.TokenCount());
		}

		BuildValueAndAdvanceMemberIndex(line[0]);

		if (activeMemberIndex >= memberTypes.size())
		{
			activeMemberIndex = 0;
			mapIndex++;
			if (mapIndex >= mapLength)
			{
				state = &CSV_Line_by_Line_SexyAssetParser::OnObjectLine;
			}
			else
			{
				state = &CSV_Line_by_Line_SexyAssetParser::OnMapKey;
			}
		}
	}

	void OnMapKey(const ICSVLine& line)
	{
		if (line.TokenCount() == 2)
		{
			if (*line[0] == '@')
			{
				builder.SetMapKey(line[1]);
				activeMemberIndex = 0;
				state = &CSV_Line_by_Line_SexyAssetParser::OnMapValue;
				return;
			}
		}

		Throw(0, "Expecting @ <map-key>");
	}

	void OnMapMember(const ICSVLine& line)
	{
		if (line.TokenCount() == 2)
		{
			if (*line[0] == '@')
			{
				OnMapKey(line);
				return;
			}
		}

		BuildMember(line);
	}

	void OnArrayMember(const ICSVLine& line)
	{
		if (line.TokenCount() == 1)
		{
			if (*line[0] == '[')
			{
				arrayIndex = 0;
				activeMemberIndex = 0;
				state = &CSV_Line_by_Line_SexyAssetParser::OnArrayValue;
				builder.SetArrayWriteIndex(0);
				return;
			}
		}

		BuildMember(line);
	}

	int32 mapLength = 0;
	int32 mapIndex = 0;
	char mapKeyType[Rococo::MAX_FQ_NAME_LEN+1];
	char mapKeySource[Rococo::IO::MAX_PATHLEN];
	char elementValueType[Rococo::MAX_FQ_NAME_LEN + 1];
	char elementValueSource[Rococo::IO::MAX_PATHLEN];

	int32 listLength = 0;
	int32 listIndex = 0;

	void OnListValue(const ICSVLine& line)
	{
		if (line.TokenCount() != 1)
		{
			Throw(0, "Expecting one member value on the line, but the token count was %llu", line.TokenCount());
		}

		BuildValueAndAdvanceMemberIndex(line[0]);

		if (activeMemberIndex >= memberTypes.size())
		{
			activeMemberIndex = 0;
			listIndex++;
			if (listIndex >= listLength)
			{
				state = &CSV_Line_by_Line_SexyAssetParser::OnObjectLine;
			}
			else
			{
				builder.AppendNewListNode(listIndex);
				state = &CSV_Line_by_Line_SexyAssetParser::OnListIndex;
			}
		}
	}

	void OnListIndex(const ICSVLine& line)
	{
		if (line.TokenCount() == 1)
		{
			if (*line[0] == '[')
			{
				state = &CSV_Line_by_Line_SexyAssetParser::OnListValue;
				return;
			}
		}

		Throw(0, "Expecting [list-index]");
	}

	void OnListMember(const ICSVLine& line)
	{
		if (line.TokenCount() == 1)
		{
			if (*line[0] == '[')
			{
				listIndex = 0;
				activeMemberIndex = 0;
				state = &CSV_Line_by_Line_SexyAssetParser::OnListValue;
				builder.AppendNewListNode(listIndex);
				return;
			}
		}

		BuildMember(line);
	}

	void OnObjectLine(const ICSVLine& line)
	{
		memberTypes.clear();

		if (line.TokenCount() == 0)
		{
			return;
		}

		fstring token0 = line[0];

		if (line.TokenCount() == 3)
		{
			cstr name = line[0];
			if (*name == '#')
			{
				if (StartsWith(line[1], "_Null_"))
				{
					builder.AddNullObject(line[0], line[1], line[2]);
				}
				else
				{
					CopyString(objectType, sizeof objectType, line[1]);
					CopyString(objectModule, sizeof objectModule, line[2]);
					builder.BuildObject(line[0], objectType, objectModule);
					state = &CSV_Line_by_Line_SexyAssetParser::OnObjectMember;
				}
				return;
			}
		}

		if (line.TokenCount() == 4)
		{
			if (*token0.buffer == '#')
			{
				if (Eq(line[1], "_SC"))
				{
					int len = atoi(line[2]);
					cstr stringText = line[3];
					builder.AddStringConstant(line[0], stringText, len);
					return;
				}
			}
			else if (StartsWith(line[0], "list"))
			{
				CopyString(elementValueType, sizeof elementValueType, line[1]);
				CopyString(elementValueSource, sizeof elementValueSource, line[2]);
				listLength = atoi(line[3]);

				CopyString(objectType, sizeof objectType, elementValueType);
				CopyString(objectModule, sizeof objectModule, elementValueSource);

				builder.AddListDefinition(line[0], elementValueType, elementValueSource, listLength);

				activeMemberIndex = 0;
				listIndex = 0;

				if (listLength == 0)
				{
					state = &CSV_Line_by_Line_SexyAssetParser::OnObjectLine;
				}
				else if (!StartsWith(elementValueType, "_Null_"))
				{
					state = &CSV_Line_by_Line_SexyAssetParser::OnListMember;
				}
				else
				{
					state = &CSV_Line_by_Line_SexyAssetParser::OnListIndex;
				}


				return;
			}
		}

		if (line.TokenCount() == 6)
		{
			if (StartsWith(line[0], "map"))
			{
				CopyString(mapKeyType, sizeof mapKeyType, line[1]);
				CopyString(mapKeySource, sizeof mapKeySource, line[2]);
				CopyString(elementValueType, sizeof elementValueType, line[3]);
				CopyString(elementValueSource, sizeof elementValueSource, line[4]);
				mapLength = atoi(line[5]);

				CopyString(objectType, sizeof objectType, elementValueType);
				CopyString(objectModule, sizeof objectModule, elementValueSource);

				builder.AddMapDefinition(line[0], line[1], line[2], line[3], line[4], mapLength);

				activeMemberIndex = 0;
				mapIndex = 0;

				if (mapLength == 0)
				{
					state = &CSV_Line_by_Line_SexyAssetParser::OnObjectLine;
				}
				else if (!StartsWith(elementValueType, "_Null_"))
				{
					state = &CSV_Line_by_Line_SexyAssetParser::OnMapMember;
				}
				else
				{
					state = &CSV_Line_by_Line_SexyAssetParser::OnMapKey;
				}


				return;
			}
		}

		if (line.TokenCount() == 5)
		{		
			if (StartsWith(line[0], "array"))
			{
				arrayLength = atoi(line[3]);
				arrayCapacity = atoi(line[4]);
				
				CopyString(objectType, sizeof objectType, line[1]);
				CopyString(objectModule, sizeof objectModule, line[2]);

				builder.AddArrayDefinition(line[0], line[1], line[2], arrayLength, arrayCapacity);

				activeMemberIndex = 0;
				arrayIndex = 0;

				if (arrayLength == 0)
				{
					state = &CSV_Line_by_Line_SexyAssetParser::OnObjectLine;
				}
				else if (!StartsWith(objectType, "_Null_"))
				{
					state = &CSV_Line_by_Line_SexyAssetParser::OnArrayMember;
				}
				else
				{
					builder.SetArrayWriteIndex(0);
					state = &CSV_Line_by_Line_SexyAssetParser::OnArrayIndex;
				}

				return;
			}

			if (*line[0] == '#')
			{
				if (Eq(line[1], "_SB"))
				{
					int32 sbLength = atoi(line[2]);
					int32 sbCapacity = atoi(line[3]);
					cstr text = line[4];

					builder.AddFastStringBuilder(line[0], fstring{ text, sbLength }, sbCapacity);
					return;
				}
			}
		}

		Throw(0, "Expecting '#<object-name> <type> <module>'\n or 'array<suffix> <element-type> <element-module> <length> <capacity>'\n or #<string-name> _SC <length> <string-text>");
	}

	void OnBadChar(Vec2i cursor, char c) override
	{
		UNUSED(cursor);
		UNUSED(c);
	}

	void OnLine(const ICSVLine& line) override
	{
		(this->*state)(line);
	}
};

namespace Rococo::IO
{
	struct CSVToken
	{
		std::vector<char> s;
	};

	// We want to cache the token array, but without having to have an object reference and be thread-safe,
	// So we use thread_local storage for this purpose.
	// In the event that the parser is adapted to be recursive, so that deserialization callbacks could, in principle invoke deserialization this mechanism will
	// produce undefined behaviour.
	thread_local std::vector<CSVToken> csv_tokens;
	thread_local size_t tokenIndex = 0;

	struct CSVLine : ICSVLine
	{
		std::vector<CSVToken>& tokens;

		int row = 0;
		size_t tokenIndex;

		CSVLine(std::vector<CSVToken>& _tokens, int _row, size_t _tokenIndex): tokens(_tokens), row(_row), tokenIndex(_tokenIndex)
		{
		}
		
		size_t TokenCount() const override
		{
			return tokenIndex;
		}

		fstring operator[] (size_t index) const override
		{
			return !tokens[index].s.empty() ? fstring{ tokens[index].s.data(), (int32) tokens[index].s.size()-1 } : ""_fstring;
		}

		int Row() const override
		{
			return row;
		}
	};

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

			int row = 0;

			void ResetBuilder()
			{
				ClearStringLiterals();
			}

			void OnBlankLine(Vec2i cursor)
			{
				UNUSED(cursor);
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
				UNUSED(row);
				UNUSED(column);

				if (startRaw)
				{
					if (tokenIndex >= csv_tokens.size())
					{
						csv_tokens.push_back(CSVToken());
					}

					auto& s = csv_tokens[tokenIndex].s;

					for (auto p = startRaw; p != endRaw; p++)
					{
						s.push_back(*p);		
					}
					s.push_back(0);
					startRaw = nullptr;
				}
				else
				{
					auto& s = csv_tokens[tokenIndex].s;
					s.push_back(0);
				}
				tokenIndex++;
			}

			void OnNewLine()
			{
				// We pass the thread-local vector to the CSVLine object to enable the debugger to give us a view into the token array.
				CSVLine line(csv_tokens, row, tokenIndex);
				
				if (tokenIndex > 0)
				{
					try
					{
						lineParser.OnLine(line);
						tokenIndex = 0;
					}
					catch (IException& ex)
					{
						Throw(ex.ErrorCode(), "Error at row %d:\n%s", row, ex.Message());
					}
				}

				ResetBuilder();
			}
		} parser{ lineParser };
		TokenizeTabbedCSVFile<LINE_PARSER>(csvString, parser);
		parser.OnNewLine();
	}

	void ParseTabbedCSV_AssetFile(cstr csvString, IMemberBuilder& builder)
	{
		CSV_Line_by_Line_SexyAssetParser parser(builder);
		ParseTabbedCSVString(csvString, parser);
	}
}
