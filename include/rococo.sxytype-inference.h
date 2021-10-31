#pragma once

#include <rococo.types.h>

namespace Rococo::Sexy
{
	struct TypeInference
	{
		Substring declarationType;
		Substring declarationVariable;
	};

	inline TypeInference TypeInference_None() { return TypeInference{ Substring_Null(), Substring_Null() }; }

	struct TypeInferenceType
	{
		char buf[256];
	};

	// An engine that attempts to infer the type of a variable by iterating through the source code that precedes it use. The source code is not required to be syntactically perfect
	class BadlyFormattedTypeInferenceEngine
	{
		cstr textBuffer;

		cstr FindFirstPrecedingChar(cstr lastChar, char match);
		cstr FindNextMatchedChar(substring_ref token, char match);
		cstr GetEndOfPadding(substring_ref token);
		cstr FindFirstLeftOccurenceOfFunctionLikeKeyword(cstr lastChar, const fstring& token);
		cstr FindLastTypeChar(substring_ref token);
		cstr FindLastVariableChar(substring_ref token);
		TypeInference FindNextPossibleDeclaration(substring_ref specimen);
		cstr GetMatchEnd(substring_ref token, cstr candidate, cstr endGuard);
	public:
		BadlyFormattedTypeInferenceEngine(cstr _textBuffer);

		TypeInference InferVariableType(substring_ref token);
		TypeInference InferContainerClass(substring_ref token);
		static void GetType(TypeInferenceType& type, const TypeInference& inference);
	};
}
