#pragma once

#include <rococo.types.h>

namespace Rococo::IO
{
	// interface to set member variables of a derivative structure. A derivative structure can be recursive, containing a hierarchy of substructures of arbitrary type
	ROCOCOAPI IMemberBuilder
	{
		// Add a boolean32 primitive
		virtual void AddBooleanMember(cstr name, bool value) = 0;

		// Add a double primitive
		virtual void AddDoubleMember(cstr name, double value) = 0;

		// Add a float primitive
		virtual void AddFloatMember(cstr name, float value) = 0;

		// Add an int32 primitive
		virtual void AddInt32Member(cstr name, int32 value) = 0;

		// Add an int64 primitive
		virtual void AddInt64Member(cstr name, int64 value) = 0;

		// Descend into derivative sub-member 
		virtual void AddDerivativeMember(cstr type, cstr name, cstr sourceFile) = 0;

		// After AddDerivativeMember, this resumes building the parent member
		virtual void ReturnToParent() = 0;
	};

	ROCOCOAPI ICSVTokenParser
	{
		virtual void OnBadChar(Vec2i cursorPosition, char value) = 0;
		virtual void OnToken(int row, int column, cstr token) = 0;
		virtual void Reset() = 0;
		virtual void Free() = 0;
	};

	ROCOCOAPI ITabbedCSVTokenizer
	{
		virtual void Tokenize(cstr csvString, ICSVTokenParser & tokenParser) = 0;
		virtual void Free() = 0;
	};

	ITabbedCSVTokenizer* CreateTabbedCSVTokenizer();

	ICSVTokenParser* CreateSXYAParser(IMemberBuilder& memberBuilder);
}
