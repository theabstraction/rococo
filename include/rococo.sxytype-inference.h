#pragma once

#include <rococo.types.h>

namespace Rococo::SexyStudio
{
	struct ISexyFieldEnumerator;
}

namespace Rococo::Sexy
{
	struct TypeInference
	{
		Substring declarationType;
		Substring declarationVariable;
	};

	inline TypeInference TypeInference_None() { return TypeInference{ Substring_Null(), Substring_Null() }; }

	// An engine that attempts to infer the type of a variable by iterating through the source code that precedes it use. The source code is not required to be syntactically perfect
	class BadlyFormattedTypeInferenceEngine
	{
		cstr textBuffer;

		cstr FindFirstPrecedingChar(cstr lastChar, char match);
		cstr FindNextMatchedChar(cr_substring token, char match);
		cstr GetEndOfPadding(cr_substring token);
		cstr FindFirstLeftOccurenceOfFunctionLikeKeyword(cstr lastChar, const fstring& token);
		cstr FindLastTypeChar(cr_substring token);
		cstr FindLastVariableChar(cr_substring token);
		TypeInference FindNextPossibleDeclaration(cr_substring specimen);
		cstr GetMatchEnd(cr_substring token, cstr candidate, cstr endGuard);
	public:
		BadlyFormattedTypeInferenceEngine(cstr _textBuffer);

		TypeInference InferLocalVariableVariableType(cr_substring token);
		TypeInference InferContainerClass(cr_substring token);
		TypeInference InferParentMember(const TypeInference& classInference, cr_substring token);
	};

	bool TryGetLocalTypeFromCurrentDocument(char type[256], bool& isThis, cr_substring candidate, cr_substring document);
	void EnumerateLocalFields(Rococo::SexyStudio::ISexyFieldEnumerator& fieldEnumerator, cstr cstrType, cr_substring file);
}
