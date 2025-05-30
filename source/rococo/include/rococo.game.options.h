#pragma once

#include <rococo.types.h>

namespace Rococo::Gui
{
	DECLARE_ROCOCO_INTERFACE IGREditFilter;
}

namespace Rococo::Game::Options
{
	ROCOCO_INTERFACE IInquiry
	{
		virtual void SetHint(cstr text) = 0;
		virtual void SetTitle(cstr title) = 0;
	};

	ROCOCO_INTERFACE IChoiceInquiry : IInquiry
	{
		// If hints are not defined they default to the choiceText
		// If they are defined at the first character is *, then they use the parent hint, append the characters trailing * and then finally append the choice text.
		virtual void AddChoice(cstr choiceName, cstr choiceText, cstr hint = nullptr) = 0;
		virtual void SetActiveChoice(cstr choiceName) = 0;
	};

	ROCOCO_INTERFACE IBoolInquiry : IInquiry
	{
		virtual void SetActiveValue(bool value) = 0;
	};

	ROCOCO_INTERFACE IScalarInquiry : IInquiry
	{
		virtual void SetActiveValue(double value) = 0;
		virtual void SetRange(double minValue, double maxValue) = 0;
	};

	ROCOCO_INTERFACE IStringInquiry : IInquiry
	{		
		virtual void SetActiveValue(cstr value) = 0;

		// Set the string filter. The filter should be valid for the lifetime of the string inquiry implementation. 
		// Setting to null will remove the inquiry's reference to a filter.
		virtual void SetFilter(Gui::IGREditFilter* filter) = 0;
	};

	ROCOCO_INTERFACE IGameOptionsBuilder
	{
		virtual IChoiceInquiry & AddChoice(cstr name) = 0;
		virtual IBoolInquiry& AddBool(cstr name) = 0;
		virtual IScalarInquiry& AddScalar(cstr name) = 0;
		// Add a string editor, maxCharacters should be something sane. GR editors put a limit on 4096 characters.
		// If the requested size is not positive or exceeds capacity an exception is thrown
		virtual IStringInquiry& AddString(cstr name, int maxCharacters) = 0;
	};

	ROCOCO_INTERFACE IOptionDatabase
	{
		virtual void Invoke(cstr name, cstr choice) = 0;
		virtual void Invoke(cstr name, bool boolValue) = 0;
		virtual void Invoke(cstr name, double scalarValue) = 0;
	};

	ROCOCO_INTERFACE IGameOptions
	{
		virtual void AddOptions(IGameOptionsBuilder & builder) = 0;
		virtual IOptionDatabase& DB() = 0;
		virtual void Accept() = 0;
		virtual void Revert() = 0;
		virtual bool IsModified() const = 0;
	};
}