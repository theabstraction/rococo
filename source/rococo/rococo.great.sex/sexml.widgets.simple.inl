#pragma once
#include <rococo.great.sex.h>
#include <sexy.types.h>
#include <Sexy.S-Parser.h>
#include <rococo.sexml.h>
#include <new>
#include <rococo.hashtable.h>
#include <rococo.functional.h>
#include <rococo.strings.h>

using namespace Rococo::Gui;
using namespace Rococo::Sex;
using namespace Rococo::Sex::SEXML;

namespace Rococo::GreatSex
{
	struct OnDivision : ISEXMLWidgetFactory
	{
		void Generate(IGreatSexGenerator& generator, const Rococo::Sex::SEXML::ISEXMLDirective& divDirective, Rococo::Gui::IGRWidget& parent) override
		{
			auto& div = Rococo::Gui::CreateDivision(parent);
			generator.SetPanelAttributes(div, divDirective);
			generator.GenerateChildren(divDirective, div);
		}
	};

	struct OnVerticalList : ISEXMLWidgetFactory
	{
		void Generate(IGreatSexGenerator& generator, const Rococo::Sex::SEXML::ISEXMLDirective& verticalListDirective, Rococo::Gui::IGRWidget& parent) override
		{
			auto& verticalList = Rococo::Gui::CreateVerticalList(parent);
			generator.SetPanelAttributes(verticalList, verticalListDirective);
			generator.GenerateChildren(verticalListDirective, verticalList);
		}
	};

	struct OnTextLabel : ISEXMLWidgetFactory
	{
		void Generate(IGreatSexGenerator& generator, const Rococo::Sex::SEXML::ISEXMLDirective& textDirective, Rococo::Gui::IGRWidget& parent) override
		{
			auto& label = Rococo::Gui::CreateText(parent);

			const fstring text = GetOptionalAttribute(textDirective, "Text", ""_fstring);
			label.SetText(text);

			//!!! Todo - implement label.SetAlignment

			generator.SetPanelAttributes(label.Widget(), textDirective);
			generator.GenerateChildren(textDirective, label.Widget());
		}
	};
}