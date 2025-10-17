// Copyright (c)2025 Mark Anthony Taylor. Email: mark.anthony.taylor@gmail.com. All rights reserved.
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
	// Example 'declarationType=Int32, declarationVariable=a, templateContainer=array'
	struct TypeInference
	{
		// Blankspace separated type names. If [templateContainer] is blank then the declarationType should be a single token
		Strings::Substring declarationType;

		Strings::Substring declarationVariable;

		// array, map, list or something else
		Strings::Substring templateContainer;
	};

	inline TypeInference TypeInference_None() { return TypeInference{ Strings::Substring::Null(), Strings::Substring::Null(), Strings::Substring::Null() }; }

	// An engine that attempts to infer the type of a variable by iterating through the source code that precedes it use. The source code is not required to be syntactically perfect
	class FaultTolerantSexyTypeInferenceEngine
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
		ROCOCO_MISC_UTILS_API FaultTolerantSexyTypeInferenceEngine(cstr _textBuffer);

		ROCOCO_MISC_UTILS_API TypeInference InferLocalVariableVariableType(Strings::cr_substring token);
		ROCOCO_MISC_UTILS_API TypeInference InferContainerClass(Strings::cr_substring token);
		ROCOCO_MISC_UTILS_API TypeInference InferParentMember(const TypeInference& classInference, Strings::cr_substring token);
	};

	ROCOCO_MISC_UTILS_API TypeInference GetLocalTypeFromCurrentDocument(bool& isThis, Strings::cr_substring candidate, Strings::cr_substring document, int depth = 0);
	ROCOCO_MISC_UTILS_API void EnumerateLocalFields(Rococo::SexyStudio::ISexyFieldEnumerator& fieldEnumerator, Strings::cr_substring searchTerm, Strings::cr_substring type, Strings::cr_substring file);
}
