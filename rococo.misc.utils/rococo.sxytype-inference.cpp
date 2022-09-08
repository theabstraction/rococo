#include <rococo.sxytype-inference.h>
#define ROCOCO_USE_SAFE_V_FORMAT
#include <rococo.strings.h>
#include <stdio.h>
#include <ctype.h>
#include <rococo.api.h>
#include <rococo.sexystudio.api.h>
#include <rococo.functional.h>

namespace
{
	using namespace Rococo;
	using namespace Rococo::Sexy;
	using namespace Rococo::Strings;

	void NullDebugFunction(const char* p)
	{

	}

	bool FindNext(Substring& cursor, cr_substring document, const fstring& token)
	{
		if (document.empty() || token.length == 0)
		{
			cursor = Substring_Null();
			return false;
		}

		if (cursor.empty())
		{
			cursor.start = document.start;
		}
		else
		{
			cursor.start = cursor.finish;
		}

		cursor.finish = cursor.start + token.length;

		while (cursor.finish < document.finish)
		{
			if (Eq(cursor, token))
			{
				return true;
			}

			cursor.start++;
			cursor.finish++;
		}

		cursor = Substring_Null();
		return false;
	}

	cstr GetClosingParenthesis(cr_substring candidate)
	{
		int count = 1;
		for (cstr p = candidate.start; p < candidate.finish; p++)
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

	// Expand t
	Substring GetClassDefinition(cr_substring className, cr_substring doc)
	{
		Substring cursor = Substring_Null();

		while (FindNext(cursor, doc, "class"_fstring))
		{
			for (cstr p = cursor.start-1; p >= doc.start; p--)
			{
				if (!isblank(*p))
				{
					if (*p == '(')
					{
						cstr name = FirstNonWhiteSpace(cursor.finish, doc.finish);
						if (!name)
						{
							return Substring_Null();
						}

						cstr lastNameChar = name + className.Length();
						if (lastNameChar > doc.finish)
						{
							return Substring_Null();
						}

						if (!Eq(Substring{ name, lastNameChar }, className))
						{
							continue;
						}

						if (!isspace(*lastNameChar) && *lastNameChar != '(')
						{
							continue;
						}

						// We matched (class <class-name> ...)
						cstr lastParenthesis = GetClosingParenthesis(Substring{ lastNameChar, doc.finish });
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
	void TEnumerateFieldsOfClassDef(cr_substring className, cr_substring classDef, T& t)
	{
		// classDef will be Name (<type1> <name1>)...(<typeN> <nameN>)

		Substring type;
		Substring name;

		bool isTypeKeyword = false;
		enum class State { ExpectingOpen, ExpectingType, ReadingType, ExpectingName, ReadingName, ExpectingClose };

		State state = State::ExpectingOpen;

		for (cstr p = classDef.start; p != classDef.finish; p++)
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
					type.finish = p;
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
					name.finish = p;
					t(type, name);
					state = State::ExpectingOpen;
					goto next;
				}
				if (isspace(c))
				{
					state = State::ExpectingClose;
					name.finish = p;
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

	void ForEachFieldOfClassDef(cr_substring className, cr_substring classDef, IFieldEnumerator& cb)
	{
		auto invokeCallback = [&cb](cr_substring name, cr_substring type)
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

	cstr BadlyFormattedTypeInferenceEngine::FindNextMatchedChar(cr_substring token, char match)
	{
		if (token.start != token.finish)
		{
			for (cstr p = token.start + 1; p < token.finish; p++)
			{
				if (*p == match)
				{
					return p;
				}
			}
		}

		return nullptr;
	}

	cstr BadlyFormattedTypeInferenceEngine::GetEndOfPadding(cr_substring token)
	{
		for (cstr q = token.start; q < token.finish; q++)
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

	cstr BadlyFormattedTypeInferenceEngine::FindLastTypeChar(cr_substring token)
	{
		for (cstr p = token.start; p != token.finish; p++)
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

	cstr BadlyFormattedTypeInferenceEngine::FindLastVariableChar(cr_substring token)
	{
		for (cstr p = token.start; p != token.finish; p++)
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

	TypeInference BadlyFormattedTypeInferenceEngine::FindNextPossibleDeclaration(cr_substring specimen)
	{
		for (cstr p = FindNextMatchedChar(specimen, '('); p != nullptr; p = FindNextMatchedChar({ p, specimen.finish }, '('))
		{
			cstr startOfType = GetEndOfPadding({ p + 1, specimen.finish });
			if (startOfType == nullptr)
			{
				continue;
			}

			if (!isupper(*startOfType))
			{
				continue;
			}

			cstr lastTypeChar = FindLastTypeChar({ startOfType, specimen.finish });
			if (lastTypeChar == nullptr)
			{
				continue;
			}

			// We will try to match ( <type> <variable>
			cstr startOfName = GetEndOfPadding({ lastTypeChar, specimen.finish });
			if (startOfName == nullptr)
			{
				continue;
			}

			if (!islower(*startOfName))
			{
				continue;
			}

			cstr endOfName = FindLastVariableChar({ startOfName, specimen.finish });

			return TypeInference{ {startOfType, lastTypeChar}, {startOfName, endOfName } };
		}

		return TypeInference{ {nullptr, nullptr},{nullptr,nullptr} };
	}

	cstr BadlyFormattedTypeInferenceEngine::GetMatchEnd(cr_substring token, cstr candidate, cstr endGuard)
	{
		cstr p;
		cstr q = candidate;
		for (p = token.start, q = candidate; p < token.finish && q < endGuard; p++, q++)
		{
			if (*p != *q)
			{
				return nullptr;
			}
		}

		return q;
	}

	TypeInference BadlyFormattedTypeInferenceEngine::InferLocalVariableVariableType(cr_substring candidate)
	{
		Substring token = candidate;
		if (token.Length() > 1)
		{
			for (cstr i = token.start; i != token.finish; ++i)
			{
				if (*i == '.')
				{
					token.finish = i;
					break;
				}
			}
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

	TypeInference BadlyFormattedTypeInferenceEngine::InferParentMember(const TypeInference& classInference, cr_substring name)
	{
		if (!classInference.declarationType) return TypeInference_None();

		Substring parentMember = name;
		for (cstr p = name.start; p != name.finish; p++)
		{
			if (*p == '.')
			{
				parentMember.finish = p;
				break;
			}
		}

		TypeInference matchType = TypeInference_None();

		auto searchForNameAndSetType = [&matchType, &parentMember](cr_substring memberType, cr_substring memberName)
		{
			if (Eq(memberName, parentMember))
			{
				matchType.declarationType = memberType;
				matchType.declarationVariable = memberName;
			}
		};

		Substring doc{ textBuffer, textBuffer + StringLength(textBuffer) };
		Substring classDef = GetClassDefinition(classInference.declarationType, doc);

		TEnumerateFieldsOfClassDef(classInference.declarationType, classDef, searchForNameAndSetType);

		return matchType;
	}

	TypeInference BadlyFormattedTypeInferenceEngine::InferContainerClass(cr_substring token)
	{
		cstr methodPos = FindFirstLeftOccurenceOfFunctionLikeKeyword(token.start, fsMethod);
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

	/* Example tokens, and the response:
	'this' and 'this.' .........................[type] the class for which the containing method applies.
	<local-variable-name>.......................the [type] defined in the containing function/method that declares the variable.
	'this.<member-variable>'....................the member [type] defined in the class for which the containing method applies
	<local-variable-name>.<children>'...........the member [type] defined in the class for which the containing method applies.
	*/
	Substring GetLocalTypeFromCurrentDocument(bool& isThis, cr_substring token, cr_substring document)
	{
		static auto thisRaw = "this"_fstring;
		static auto thisDot = "this."_fstring;

		if (!token || !document || !islower(*token.start))
		{
			return Substring::Null();
		}

		using namespace Rococo::Sexy;

		Substring searchTerm = token;

		BadlyFormattedTypeInferenceEngine engine(document.start);

		if (Eq(token, thisRaw)) // 'this'
		{
			isThis = true;
			auto classInference = engine.InferContainerClass(token);
			return classInference.declarationType;
		}
		else if (Eq(token, thisDot)) // 'this.'
		{
			isThis = true;
			auto classInference = engine.InferContainerClass({ token.start, token.finish - 1 });
			return classInference.declarationType;
		}
		else if (StartsWith(token, thisDot)) // 'this.<member-variable>'
		{
			auto classInference = engine.InferContainerClass({ token.start, token.start + thisRaw.length });
			isThis = false;
			Substring memberName{ token.start + thisDot.length,token.finish };
			auto memberInference = engine.InferParentMember(classInference, memberName);
			return memberInference.declarationType;
		}
		else
		{
			// a.b.c.d -> should return typeof(a)
			isThis = false;
			auto localVariableInference =  engine.InferLocalVariableVariableType(searchTerm);

			if (localVariableInference.declarationType)
			{
				return localVariableInference.declarationType;
			}
			else
			{
				return Substring::Null();
			}
		}
	}

	EFlowLogic PassFieldToEnumerator(SexyStudio::ISexyFieldEnumerator& fieldEnumerator, cr_substring searchTerm, cr_substring fieldDef)
	{
		if (fieldDef.empty())
		{
			return EFlowLogic::CONTINUE;
		}

		// We have a field def, in which we have 2 entries (<Field-Type> <field-name>), with our substring begin and start within the parenthesis, not including them
		cstr fieldTypeStart = SkipBlankspace(fieldDef);
		if (fieldTypeStart == fieldDef.finish)
		{
			// It was an empty expression
			return EFlowLogic::CONTINUE;
		}

		cstr fieldEnd = SkipNotBlankspace(Substring{ fieldTypeStart, fieldDef.finish });
		if (fieldEnd == fieldDef.finish)
		{
			// We only had one contiguous string
			return EFlowLogic::CONTINUE;
		}

		Substring fieldType{ fieldTypeStart, fieldEnd };

		if (IsLowerCase(*fieldTypeStart))
		{
			if (Eq(fieldType, "array"_fstring) || Eq(fieldType, "map"_fstring) || Eq(fieldType, "list"_fstring))
			{
				// Container, type is in position 1 (<container> <Field-type> <field-name> <fields...>)
				fieldTypeStart = SkipBlankspace(Substring{ fieldEnd, fieldDef.finish });
				if (fieldTypeStart == fieldDef.finish)
				{
					// We are missing something
					return EFlowLogic::CONTINUE;
				}

				fieldEnd = SkipNotBlankspace(Substring{ fieldTypeStart, fieldDef.finish });
				if (fieldEnd == fieldDef.finish)
				{
					// We only had one contiguous string
					return EFlowLogic::CONTINUE;
				}
				else
				{
					fieldType = { fieldTypeStart, fieldEnd };
				}
			}
			else
			{
				// Cannot determine what this is meant to be
				return EFlowLogic::CONTINUE;
			}
		}

		if (!IsCapital(*fieldTypeStart) || !IsAlphaNumeric(fieldType))
		{
			// Field was not a type
			return EFlowLogic::CONTINUE;
		}

		Substring fieldRight{ fieldEnd, fieldDef.finish };

		cstr fieldNameStart = SkipBlankspace(fieldRight);
		if (fieldNameStart == fieldDef.finish)
		{
			// No field name was specified
			return EFlowLogic::CONTINUE;
		}

		cstr fieldNameEnd = SkipNotBlankspace(Substring{ fieldNameStart, fieldDef.finish });

		if (!IsLowerCase(*fieldNameStart))
		{
			// Field was not an identifier
			return EFlowLogic::CONTINUE;
		}

		Substring fieldName{ fieldNameStart, fieldNameEnd };
		if (!IsAlphaNumeric(fieldName) || !IsLowerCase(*fieldNameStart))
		{
			return EFlowLogic::CONTINUE;
		}

		if (Eq(searchTerm, fieldName))
		{
			fieldEnumerator.OnFieldType(fieldType, fieldName);
			return EFlowLogic::BREAK;
		}

		cstr firstDot = Strings::ForwardFind('.', searchTerm);
		if (!firstDot)
		{
			return EFlowLogic::CONTINUE;
		}

		cstr nextDot = Strings::ForwardFind('.', { firstDot + 1, searchTerm.finish });
		if (!nextDot)
		{
			char result[128];
			Strings::CopyWithTruncate(fieldName, result, sizeof result);
			fieldEnumerator.OnField(result);
			return EFlowLogic::CONTINUE;
		}

		char fieldNameArray[128];
		Strings::CopyWithTruncate(fieldName, fieldNameArray, sizeof fieldNameArray);

		Substring searchTail{ firstDot + 1, nextDot };

		if (!Strings::FindSubstring(searchTail, to_fstring(fieldNameArray)))
		{
			// Mismatch of field name vs search term
			return EFlowLogic::CONTINUE;
		}

		fieldEnumerator.OnFieldType(fieldType, searchTail);

		return EFlowLogic::BREAK;
	}

	void EnumerateLocalFieldsOfCandidate(SexyStudio::ISexyFieldEnumerator& fieldEnumerator, cr_substring searchTerm, cr_substring candidate, cr_substring file)
	{
		// We found some type name TYPE in the file, we need to see if it is part of a type definition, e.g (struct <TYPE> <fields>)

		if (candidate.empty()) return;

		cstr firstParenthesisToLeft = ReverseFind('(', Substring{ file.start, candidate.start });

		if (firstParenthesisToLeft == nullptr)
		{
			// Type definition requires an open parenthesis
			return;
		}

		Substring leftChars = Substring{ firstParenthesisToLeft + 1, candidate.start - 1 };
		if (leftChars.empty())
		{
			return;
		}

		cstr nextParenthesis = ForwardFind(')', leftChars);
		if (nextParenthesis)
		{
			// Type definition requires no close parenthesis before the open parenthesis to its left
			return;
		}

		if (!FindSubstring(leftChars, "struct"_fstring) && !FindSubstring(leftChars, "class"_fstring))
		{
			// Neither a struct nor a class definition
			return;
		}

		// Now we have some sort of potentially badly formatted struct or class of this form:
		// (<irrelevant> struct <irrelevant> <TYPE> <fields>) where each field is of form (<Field-Type> <field-name>)

		Substring rightChars{ candidate.finish, file.finish };

		if (*candidate.finish != '(' && !IsBlankspace(*candidate.finish))
		{
			// We found a prefix, rather than the token
			return;
		}

		for (;;)
		{
			if (rightChars.empty())
			{
				return;
			}

			cstr nextFieldOpener = ForwardFind('(', rightChars);
			if (nextFieldOpener == nullptr)
			{
				return;
			}

			cstr nextFieldCloser = ForwardFind(')', rightChars);
			if (nextFieldCloser < nextFieldOpener)
			{
				// This means we hit the definition close, i.e (struct <TYPE> (Int32 field) ****)****
				return;
			}

			EFlowLogic logic = PassFieldToEnumerator(fieldEnumerator, searchTerm, Substring{ nextFieldOpener + 1, nextFieldCloser });
			if (logic == EFlowLogic::BREAK)
			{
				break;
			}

			rightChars.start = nextFieldCloser + 1;
		}
	}

	void EnumerateLocalFields(SexyStudio::ISexyFieldEnumerator& fieldEnumerator, cr_substring searchTerm, cr_substring type, cr_substring file)
	{
		if (type.Length() < 3)
		{
			return;
		}

		if (!IsCapital(type.start[0]))
		{
			return;
		}

		auto evalCandidate = [&fieldEnumerator, &file, &type, &searchTerm](cr_substring candidate)
		{
			if (!IsAlphaNumeric(*candidate.finish))
			{
				EnumerateLocalFieldsOfCandidate(fieldEnumerator, searchTerm, candidate, file);
			}
		};

		ForEachOccurence(file, type, evalCandidate);
	}
} // Rococo::Sexy