#pragma once

#include <rococo.types.h>

struct IVarArgStringFormatter;

namespace Rococo
{
	ROCOCO_INTERFACE IFieldEnumerator
	{
		virtual void OnMemberVariable(cstr name, cstr type) = 0;
	};
}

namespace Rococo::Sexy
{
	using namespace Rococo::Strings;
	// Type inference API
	// TODO - move functions to their own header
	ROCOCO_MISC_UTILS_API void ForEachFieldOfClassDef(cr_substring className, cr_substring classDef, IFieldEnumerator& cb);
	ROCOCO_MISC_UTILS_API Substring GetClassDefinition(cr_substring className, cr_substring doc);
	ROCOCO_API bool IsSexyKeyword(cr_substring candidate);
	ROCOCO_API bool IsNotTokenChar(char c);
	ROCOCO_API cstr GetFirstNonTokenPointer(cr_substring s);
	ROCOCO_API cstr GetFirstNonTypeCharPointer(cr_substring s);
	ROCOCO_API Substring GetFirstTokenFromLeft(cr_substring s);
	// Given a document and a position to the right of the start of the doc, return first pointer of none type char found, or null if everything was of type until doc.start
	ROCOCO_API cstr GetFirstNonTokenPointerFromRight(cr_substring doc, cstr startPosition);
}