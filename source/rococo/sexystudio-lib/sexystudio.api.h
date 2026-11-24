#pragma once

// SexyStudio database S-expression parse helpers (ParseKeyword, AtomicArg, match_compound), used by sexystudio-lib.
// This header is part of the GUI-free database layer: it must not include or forward declare window, widget or IDE types.

#include <rococo.types.h>

namespace Rococo
{
	namespace Sex
	{
		bool IsCompound(Sex::cr_sex s);
	}
}

namespace Rococo::SexyStudio
{
	struct AtomicArg
	{
		bool Matches(Sex::cr_sex s, int index) const;
		fstring operator()(Sex::cr_sex s, int index) const;
	};

	class ParseKeyword
	{
		fstring keyword;
	public:
		ParseKeyword(cstr _keyword);
		bool Matches(Sex::cr_sex s, int index) const;
		fstring operator()(Sex::cr_sex s, int index) const;
	};

	extern ParseKeyword keywordNamespace;
	extern ParseKeyword keywordInterface;
	extern ParseKeyword keywordStruct;
	extern ParseKeyword keywordFunction;
	extern ParseKeyword keywordMacro;
	extern ParseKeyword keywordAlias;
	extern AtomicArg ParseAtomic;

	/* Two functions to allow manipulation of ISExpression's without having to include sexy headers*/

	int Len(Sex::cr_sex s);

	template<class ACTION, class FIRSTARG, class SECONDARG>
	inline bool match_compound(Sex::cr_sex s, int nMaxArgs, FIRSTARG a, SECONDARG b, ACTION action)
	{
		if (!IsCompound(s)) return false;
		if (Len(s) < 2) return false;
		if (Len(s) > nMaxArgs) return false;

		if (!a.Matches(s, 0)) return false;
		if (!b.Matches(s, 1)) return false;

		action(s, a(s, 0), b(s, 1));

		return true;
	}

	template<class ACTION, class FIRSTARG, class SECONDARG, class THIRDARG>
	inline bool match_compound(Sex::cr_sex s, int nMaxArgs, FIRSTARG a, SECONDARG b, THIRDARG c, ACTION action)
	{
		if (!IsCompound(s)) return false;
		if (Len(s) < 3) return false;
		if (Len(s) > nMaxArgs) return false;

		if (!a.Matches(s, 0)) return false;
		if (!b.Matches(s, 1)) return false;
		if (!c.Matches(s, 2)) return false;

		action(s, a(s, 0), b(s, 1), c(s, 2));

		return true;
	}
}
