#pragma once

#include <rococo.types.h>

#ifndef ROCOCO_MISC_UTILS_API
# define ROCOCO_MISC_UTILS_API ROCOCO_API_IMPORT
#endif

namespace Rococo::SexyStudio
{
	struct ISexyFieldEnumerator;
}

namespace Rococo::Sex::Inference
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

		ROCOCO_MISC_UTILS_API cstr FindFirstPrecedingChar(cstr lastChar, char match);
		ROCOCO_MISC_UTILS_API cstr FindNextMatchedChar(Strings::cr_substring token, char match);
		ROCOCO_MISC_UTILS_API cstr GetEndOfPadding(Strings::cr_substring token);
		ROCOCO_MISC_UTILS_API cstr FindFirstLeftOccurenceOfFunctionLikeKeyword(cstr lastChar, const fstring& token);
		ROCOCO_MISC_UTILS_API cstr FindLastTypeChar(Strings::cr_substring token);
		ROCOCO_MISC_UTILS_API cstr FindLastVariableChar(Strings::cr_substring token);
		ROCOCO_MISC_UTILS_API TypeInference FindNextPossibleDeclaration(Strings::cr_substring specimen);
		ROCOCO_MISC_UTILS_API cstr GetMatchEnd(Strings::cr_substring token, cstr candidate, cstr endGuard);
	public:
		ROCOCO_MISC_UTILS_API BadlyFormattedTypeInferenceEngine(cstr _textBuffer);

		ROCOCO_MISC_UTILS_API TypeInference InferLocalVariableVariableType(Strings::cr_substring token);
		ROCOCO_MISC_UTILS_API TypeInference InferContainerClass(Strings::cr_substring token);
		ROCOCO_MISC_UTILS_API TypeInference InferParentMember(const TypeInference& classInference, Strings::cr_substring token);
	};

	ROCOCO_MISC_UTILS_API Strings::Substring GetLocalTypeFromCurrentDocument(bool& isThis, Strings::cr_substring candidate, Strings::cr_substring document);
	ROCOCO_MISC_UTILS_API void EnumerateLocalFields(Rococo::SexyStudio::ISexyFieldEnumerator& fieldEnumerator, Strings::cr_substring searchTerm, Strings::cr_substring type, Strings::cr_substring file);
}
