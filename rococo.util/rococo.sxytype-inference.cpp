#include <rococo.sxytype-inference.h>
#define ROCOCO_USE_SAFE_V_FORMAT
#include <rococo.strings.h>
#include <stdio.h>
#include <ctype.h>
#include <rococo.api.h>

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

	TypeInference BadlyFormattedTypeInferenceEngine::InferVariableType(substring_ref token)
	{
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