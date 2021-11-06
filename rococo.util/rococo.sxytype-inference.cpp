#include <rococo.sxytype-inference.h>
#define ROCOCO_USE_SAFE_V_FORMAT
#include <rococo.strings.h>
#include <stdio.h>
#include <ctype.h>
#include <rococo.api.h>

namespace
{
	using namespace Rococo;
	using namespace Rococo::Sexy;

	void NullDebugFunction(const char* p)
	{

	}

	bool FindNext(Substring& cursor, substring_ref document, cstr token)
	{
		if (document.empty() || *token == 0)
		{
			cursor = Substring_Null();
			return false;
		}

		fstring fsToken = to_fstring(token);

		if (cursor.empty())
		{
			cursor.start = document.start;
		}

		cursor.end = cursor.start + fsToken.length;

		while (cursor.end < document.end)
		{
			if (Eq(cursor, fsToken))
			{
				return true;
			}

			cursor.start++;
			cursor.end++;
		}

		cursor = Substring_Null();
		return false;
	}

	cstr GetClosingParenthesis(substring_ref candidate)
	{
		int count = 1;
		for (cstr p = candidate.start; p < candidate.end; p++)
		{
			if (*p == '(')
			{
				count++;
			}
			else if (*p == ')')
			{
				count--;

				if (count == 0)
				{
					return p;
				}
			}
		}

		return nullptr;
	}

	cstr FirstNonWhiteSpace(cstr start, cstr end)
	{
		for (cstr p = start; p < end; p++)
		{
			if (!isspace(*p))
			{
				return p;
			}
		}

		return nullptr;
	}
}

namespace Rococo::Sexy
{
	bool IsBlankspace(char c)
	{
		switch (c)
		{
		case ' ':
		case '\t':
		case '\r':
		case '\n':
			return true;
		default:
			return false;
		}
	}
	
	static const fstring fsFunction = "function"_fstring;
	static const fstring fsMethod = "method"_fstring;

	// Expamd t
	Substring GetClassDefinition(cstr className, substring_ref doc)
	{
		auto fsName = to_fstring(className);
		Substring cursor = Substring_Null();

		while (FindNext(cursor, doc, "class"))
		{
			for (cstr p = cursor.start - 1; p > doc.start; p--)
			{
				if (!isblank(*p))
				{
					if (*p == '(')
					{
						cstr name = FirstNonWhiteSpace(cursor.end, doc.end);
						if (!name)
						{
							return Substring_Null();
						}

						cstr lastNameChar = name + StringLength(className);
						if (lastNameChar > doc.end)
						{
							return Substring_Null();
						}

						if (!Eq(Substring{ name, lastNameChar }, fsName))
						{
							continue;
						}

						if (!isspace(*lastNameChar) && *lastNameChar != '(')
						{
							continue;
						}

						// We matched (class <class-name> ...)
						cstr lastParenthesis = GetClosingParenthesis(Substring{ lastNameChar, doc.end });
						if (lastParenthesis)
						{
							Substring classDef{ lastNameChar + 1, lastParenthesis - 1 };
							return classDef;
						}
					}
				}
			}
		}

		return Substring_Null();
	}

	template<class T>
	void TEnumerateFieldsOfClassDef(cstr className, substring_ref classDef, T& t)
	{
		// classDef will be Name (<type1> <name1>)...(<typeN> <nameN>)

		Substring type;
		Substring name;

		bool isTypeKeyword = false;
		enum class State { ExpectingOpen, ExpectingType, ReadingType, ExpectingName, ReadingName, ExpectingClose };

		State state = State::ExpectingOpen;

		for (cstr p = classDef.start; p != classDef.end; p++)
		{
			char c = *p;

			switch (state)
			{
			case State::ExpectingOpen:
				if (isspace(c))
				{
					goto next;
				}

				switch (c)
				{
				case '(':
					state = State::ExpectingType;
					goto next;
				default:
					// Unexpected character - skip it
					goto next;
				}
			case State::ExpectingType:
				if (isspace(c))
				{
					goto next;
				}

				if (isupper(c))
				{
					type.start = p;
					isTypeKeyword = false;
					state = State::ReadingType;
					goto next;
				}

				if (islower(c))
				{
					type.start = p;
					isTypeKeyword = true;
					state = State::ReadingType;
					goto next;
				}

				// Unexpected character - skip it
				goto next;
			case State::ReadingType:
				if (isspace(c))
				{
					state = State::ExpectingName;
					type.end = p;
					goto next;
				}

				if (!isalnum(c))
				{
					// Unexpected character
					return;
				}

				goto next;
			case State::ExpectingName:
				if (isspace(c))
				{
					goto next;
				}
				if (!isTypeKeyword && islower(c))
				{
					state = State::ReadingName;
					name.start = p;
					goto next;
				}
				else if (isTypeKeyword && isalnum(c))
				{
					if (Eq(type, "array"_fstring))
					{
						type.start = p;
						state = State::ReadingType;
						goto next;
					}
					state = State::ReadingName;
					name.start = p;
					goto next;
				}
				// Unexpected character
				return;
			case State::ReadingName:
				if (isalnum(c))
				{
					goto next;
				}
				if (c == ')')
				{
					name.end = p;
					t(type, name);
					state = State::ExpectingOpen;
					goto next;
				}
				if (isspace(c))
				{
					state = State::ExpectingClose;
					name.end = p;
					goto next;
				}
				else
				{
					// Unexpected character
					return;
				}
			case State::ExpectingClose:
				if (isspace(c))
				{
					goto next;
				}
				if (c == ')')
				{
					t(type, name);
					state = State::ExpectingOpen;
					goto next;
				}
				else
				{
					// Unexpected character
					return;
				}
			}

		next:
			NullDebugFunction(p);
		}
	}

	void ForEachFieldOfClassDef(cstr className, substring_ref classDef, IFieldEnumerator& cb)
	{
		auto invokeCallback = [&cb](substring_ref name, substring_ref type)
		{
			char nameBuffer[128];
			char typeBuffer[128];
			CopyWithTruncate(name, nameBuffer, sizeof nameBuffer);
			CopyWithTruncate(type, typeBuffer, sizeof typeBuffer);
			cb.OnMemberVariable(typeBuffer, nameBuffer);
		};
		TEnumerateFieldsOfClassDef(className, classDef, invokeCallback);
	}

	// An engine that attempts to infer the type of a variable by iterating through the source code that precedes it use. The source code is not required to be synatically perfect

	BadlyFormattedTypeInferenceEngine::BadlyFormattedTypeInferenceEngine(cstr _textBuffer) : textBuffer(_textBuffer)
	{

	}

	cstr BadlyFormattedTypeInferenceEngine::FindFirstPrecedingChar(cstr lastChar, char match)
	{
		for (cstr p = lastChar - 1; p >= textBuffer; p--)
		{
			if (*p == match)
			{
				return p;
			}
		}

		return nullptr;
	}

	cstr BadlyFormattedTypeInferenceEngine::FindNextMatchedChar(substring_ref token, char match)
	{
		if (token.start != token.end)
		{
			for (cstr p = token.start + 1; p < token.end; p++)
			{
				if (*p == match)
				{
					return p;
				}
			}
		}

		return nullptr;
	}

	cstr BadlyFormattedTypeInferenceEngine::GetEndOfPadding(substring_ref token)
	{
		for (cstr q = token.start; q < token.end; q++)
		{
			if (!IsBlankspace(*q))
			{
				return q;
			}
		}

		return nullptr;
	}

	cstr BadlyFormattedTypeInferenceEngine::FindFirstLeftOccurenceOfFunctionLikeKeyword(cstr lastChar, const fstring& token)
	{
		// Expecting a ( function   <f-name> ...). If a match is found then the return value will point to <f-name
		for (cstr p = FindFirstPrecedingChar(lastChar, '('); p != nullptr; p = FindFirstPrecedingChar(p, '('))
		{
			cstr endOfPadding = GetEndOfPadding({ p + 1, lastChar });
			if (endOfPadding == nullptr)
			{
				continue;
			}

			if (StrCmpN(endOfPadding, token, token.length) == 0)
			{
				cstr endOfFunctionkeyword = endOfPadding + token.length + 1;
				cstr endOfFunctionkeywordWithPadding = GetEndOfPadding({ endOfFunctionkeyword, lastChar });
				if (endOfFunctionkeywordWithPadding == nullptr)
				{
					continue;
				}
				else
				{
					return endOfFunctionkeywordWithPadding;
				}
			}
		}

		return nullptr;
	}

	cstr BadlyFormattedTypeInferenceEngine::FindLastTypeChar(substring_ref token)
	{
		for (cstr p = token.start; p != token.end; p++)
		{
			if (isalnum(*p) || *p == '.')
			{
				continue;
			}
			else if (*p == '(' || *p == ')')
			{
				return nullptr;
			}
			else if (IsBlankspace(*p))
			{
				return p;
			}
			else
			{
				return nullptr;
			}
		}

		return nullptr;
	}

	cstr BadlyFormattedTypeInferenceEngine::FindLastVariableChar(substring_ref token)
	{
		for (cstr p = token.start; p != token.end; p++)
		{
			if (isalnum(*p))
			{
				continue;
			}
			else if (*p == '(' || *p == ')')
			{
				return p;
			}
			else if (IsBlankspace(*p))
			{
				return p;
			}
			else
			{
				return nullptr;
			}
		}

		return nullptr;
	}

	TypeInference BadlyFormattedTypeInferenceEngine::FindNextPossibleDeclaration(substring_ref specimen)
	{
		for (cstr p = FindNextMatchedChar(specimen, '('); p != nullptr; p = FindNextMatchedChar({ p, specimen.end }, '('))
		{
			cstr startOfType = GetEndOfPadding({ p + 1, specimen.end });
			if (startOfType == nullptr)
			{
				continue;
			}

			if (!isupper(*startOfType))
			{
				continue;
			}

			cstr lastTypeChar = FindLastTypeChar({ startOfType, specimen.end });
			if (lastTypeChar == nullptr)
			{
				continue;
			}

			// We will try to match ( <type> <variable>
			cstr startOfName = GetEndOfPadding({ lastTypeChar, specimen.end });
			if (startOfName == nullptr)
			{
				continue;
			}

			if (!islower(*startOfName))
			{
				continue;
			}

			cstr endOfName = FindLastVariableChar({ startOfName, specimen.end });

			return TypeInference{ {startOfType, lastTypeChar}, {startOfName, endOfName } };
		}

		return TypeInference{ {nullptr, nullptr},{nullptr,nullptr} };
	}

	cstr BadlyFormattedTypeInferenceEngine::GetMatchEnd(substring_ref token, cstr candidate, cstr endGuard)
	{
		cstr p;
		cstr q = candidate;
		for (p = token.start, q = candidate; p < token.end && q < endGuard; p++, q++)
		{
			if (*p != *q)
			{
				return nullptr;
			}
		}

		return q;
	}

	TypeInference BadlyFormattedTypeInferenceEngine::InferParentMember(const TypeInference& classInference, substring_ref name)
	{
		if (!classInference.declarationType) return TypeInference_None();

		Substring parentMember = name;
		for (cstr p = name.start; p != name.end; p++)
		{
			if (*p == '.')
			{
				parentMember.end = p;
				break;
			}
		}

		TypeInference matchType = TypeInference_None();

		auto searchForNameAndSetType = [&matchType, &parentMember](substring_ref memberType, substring_ref memberName)
		{
			if (Eq(memberName, parentMember))
			{
				matchType.declarationType = memberType;
				matchType.declarationVariable = memberName;
			}
		};

		char classNameBuffer[128];
		CopyWithTruncate(classInference.declarationType, classNameBuffer, sizeof classNameBuffer);

		Substring doc{ textBuffer, textBuffer + StringLength(textBuffer) };
		Substring classDef = GetClassDefinition(classNameBuffer, doc);

		TEnumerateFieldsOfClassDef(classNameBuffer, classDef, searchForNameAndSetType);

		return matchType;
	}

	TypeInference BadlyFormattedTypeInferenceEngine::InferContainerClass(substring_ref token)
	{
		cstr methodPos = FindFirstLeftOccurenceOfFunctionLikeKeyword(token.start - fsMethod.length, fsMethod);
		if (!methodPos)
		{
			return TypeInference_None();
		}

		// methodPos = <classname>.<methodname> (args ...)-> : (this.... )

		for (cstr p = methodPos; p < token.start; p++)
		{
			if (*p == '.')
			{
				return TypeInference { Substring{ methodPos, p }, token };
			}
		}


		return TypeInference_None();
	}

	TypeInference BadlyFormattedTypeInferenceEngine::InferParentVariableType(substring_ref token)
	{
		if (Eq(token, "this"_fstring))
		{
			return InferContainerClass(token);
		}
		else if (Length(token) > 5 && StartsWith(token, "this."_fstring))
		{
			Substring thisToken = { token.start, token.start + 4 };
			TypeInference classType = InferContainerClass(thisToken);
			return InferParentMember(classType, { thisToken.end+1, token.end });
		}

		cstr endGuard = max(textBuffer, token.start - 1); // We want to go back a bit, because we dont expect to find '(function' immediately before the candidate

		// Algorithm -> iterate backwards from tokenBegin and search for '(method' or '(function' to identify the containing function where the type is defined
		cstr functionPos = FindFirstLeftOccurenceOfFunctionLikeKeyword(token.start - fsFunction.length, fsFunction);
		if (functionPos == nullptr)
		{
			functionPos = FindFirstLeftOccurenceOfFunctionLikeKeyword(token.start - fsMethod.length, fsMethod);
		}

		if (functionPos == nullptr)
		{
			return TypeInference{ {nullptr, nullptr},{nullptr, nullptr} };
		}

		// A variable could be declared as input, output or local variable, we advance and search for (<type> <variable>...)
		for (auto d = FindNextPossibleDeclaration({ functionPos, endGuard }); !IsEmpty(d.declarationType); d = FindNextPossibleDeclaration({ d.declarationType.start, endGuard }))
		{
			if (Eq(d.declarationVariable, token))
			{
				return d;
			}
		}

		return TypeInference{ {nullptr, nullptr},{nullptr, nullptr} };
	}

	void BadlyFormattedTypeInferenceEngine::GetType(TypeInferenceType& type, const TypeInference& inference)
	{
		char* q = type.buf;

		if (sizeof TypeInferenceType > Length(inference.declarationType))
		{
			for (cstr p = inference.declarationType.start; p < inference.declarationType.end; p++, q++)
			{
				*q = *p;
			}
		}

		*q = 0;
	}
} // Rococo::Sexy