#pragma once

#include <rococo.types.h>

#ifndef ROCOCO_MISC_UTILS_API
# define ROCOCO_MISC_UTILS_API ROCOCO_API_IMPORT
#endif

namespace Rococo::SexyStudio
{
	struct ISexyFieldEnumerator;
}

namespace Rococo::Sexy
{
	struct TypeInference
	{
		Strings::Substring declarationType;
		Strings::Substring declarationVariable;
	};

	inline TypeInference TypeInference_None() { return TypeInference{ Strings::Substring_Null(), Strings::Substring_Null() }; }

	// An engine that attempts to infer the type of a variable by iterating through the source code that precedes it use. The source code is not required to be syntactically perfect
	class BadlyFormattedTypeInferenceEngine
	{
		cstr textBuffer;

		cstr FindFirstPrecedingChar(cstr lastChar, char match);
		cstr FindNextMatchedChar(Strings::cr_substring token, char match);
		cstr GetEndOfPadding(Strings::cr_substring token);
		cstr FindFirstLeftOccurenceOfFunctionLikeKeyword(cstr lastChar, const fstring& token);
		cstr FindLastTypeChar(Strings::cr_substring token);
		cstr FindLastVariableChar(Strings::cr_substring token);
		TypeInference FindNextPossibleDeclaration(Strings::cr_substring specimen);
		cstr GetMatchEnd(Strings::cr_substring token, cstr candidate, cstr endGuard);
	public:
		BadlyFormattedTypeInferenceEngine(cstr _textBuffer);

		TypeInference InferLocalVariableVariableType(Strings::cr_substring token);
		TypeInference InferContainerClass(Strings::cr_substring token);
		TypeInference InferParentMember(const TypeInference& classInference, Strings::cr_substring token);
	};

	ROCOCO_MISC_UTILS_API Strings::Substring GetLocalTypeFromCurrentDocument(bool& isThis, Strings::cr_substring candidate, Strings::cr_substring document);
	ROCOCO_MISC_UTILS_API void EnumerateLocalFields(Rococo::SexyStudio::ISexyFieldEnumerator& fieldEnumerator, Strings::cr_substring searchTerm, Strings::cr_substring type, Strings::cr_substring file);
}
