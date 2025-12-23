#pragma once

// The SexyStudio Widget API. This file should be kept free of OS dependent data structures and functions
// Widgets ineract with OS windows via Windows::IWindow interface on their Window method

#include <rococo.api.h>
#include <rococo.events.h>
#include <rococo.time.h>

namespace Rococo
{
	namespace Events
	{
		struct EventIdRef;
		class IPublisher;
	}

	namespace Sex
	{
		bool IsCompound(Sex::cr_sex s);
	}
}

namespace Rococo::SexyStudio
{
	struct IGuiWidget;
	struct IWidgetSet;
	struct ISexyStudioEventHandler;

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
