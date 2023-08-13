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

namespace Rococo::Sex::Inference
{
	// Type inference API
	// TODO - move functions to their own header
	ROCOCO_MISC_UTILS_API void ForEachFieldOfClassDef(Rococo::Strings::cr_substring className, Rococo::Strings::cr_substring classDef, IFieldEnumerator& cb);
	ROCOCO_MISC_UTILS_API Rococo::Strings::Substring GetClassDefinition(Rococo::Strings::cr_substring className, Rococo::Strings::cr_substring doc);
	ROCOCO_API bool IsSexyKeyword(Rococo::Strings::cr_substring candidate);
	ROCOCO_API bool IsNotTokenChar(char c);
	ROCOCO_API cstr GetFirstNonTokenPointer(Rococo::Strings::cr_substring s);
	ROCOCO_API cstr GetFirstNonTypeCharPointer(Rococo::Strings::cr_substring s);
	ROCOCO_API Rococo::Strings::Substring GetFirstTokenFromLeft(Rococo::Strings::cr_substring s);
	// Given a document and a position to the right of the start of the doc, return first pointer of none type char found, or null if everything was of type until doc.start
	ROCOCO_API cstr GetFirstNonTokenPointerFromRight(Rococo::Strings::cr_substring doc, cstr startPosition);
}