#pragma once

#include <rococo.types.h>

namespace Rococo::Game::Options
{
	ROCOCO_INTERFACE IInqiury
	{
		virtual void SetTitle(cstr title) = 0;
	};

	ROCOCO_INTERFACE IChoiceInquiry : IInqiury
	{
		virtual void AddChoice(cstr choiceName, cstr choiceText) = 0;
		virtual void SetActiveChoice(cstr choiceName) = 0;
	};

	ROCOCO_INTERFACE IBoolInquiry : IInqiury
	{
		virtual void SetActiveValue(bool value) = 0;
	};

	ROCOCO_INTERFACE IScalarInquiry : IInqiury
	{
		virtual void SetActiveValue(double value) = 0;
		virtual void SetRange(double minValue, double maxValue) = 0;
	};

	ROCOCO_INTERFACE IGameOptionsBuilder
	{
		virtual IChoiceInquiry & AddChoice(cstr name) = 0;
		virtual IBoolInquiry& AddBool(cstr name) = 0;
		virtual IScalarInquiry& AddScalar(cstr name) = 0;
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
	};
}