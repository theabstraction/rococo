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
	struct DivisionFactory : ISEXMLWidgetFactory
	{
		void Generate(IGreatSexGenerator& generator, const Rococo::Sex::SEXML::ISEXMLDirective& divDirective, Rococo::Gui::IGRWidget& parent) override
		{
			auto& div = Rococo::Gui::CreateDivision(parent);
			generator.SetPanelAttributes(div, divDirective);
			generator.GenerateChildren(divDirective, div);
		}
	};

	struct VerticalListFactory : ISEXMLWidgetFactory
	{
		void Generate(IGreatSexGenerator& generator, const Rococo::Sex::SEXML::ISEXMLDirective& verticalListDirective, Rococo::Gui::IGRWidget& parent) override
		{
			auto& verticalList = Rococo::Gui::CreateVerticalList(parent);
			generator.SetPanelAttributes(verticalList.Widget(), verticalListDirective);
			generator.GenerateChildren(verticalListDirective, verticalList.Widget());
		}
	};

	struct TextLabelFactory : ISEXMLWidgetFactory
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

	struct ButtonFactory : ISEXMLWidgetFactory
	{
		void Generate(IGreatSexGenerator& generator, const Rococo::Sex::SEXML::ISEXMLDirective& buttonDirective, Rococo::Gui::IGRWidget& parent) override
		{
			auto& button = Rococo::Gui::CreateButton(parent);
			
			const fstring text = GetOptionalAttribute(buttonDirective, "Text", ""_fstring);
			button.SetTitle(text);

			generator.SetPanelAttributes(button.Widget(), buttonDirective);
			generator.GenerateChildren(buttonDirective, button.Widget());
		}
	};

	uint64 ParseAlignment(const fstring& token)
	{
		MATCH(token, "left", (uint64) EGRAlignment::Left);
		MATCH(token, "top", (uint64) EGRAlignment::Top);
		MATCH(token, "right", (uint64) EGRAlignment::Right);
		MATCH(token, "bottom", (uint64) EGRAlignment::Bottom);
		MATCH(token, "hcentre", (uint64) EGRAlignment::HCentre);
		MATCH(token, "vcentre", (uint64) EGRAlignment::VCentre);
		Throw(0, "Unknown alignment - %s. Expecting one of { left, top, right, bottom, hcentre, vcentre }", token.buffer);
	}

	EGRAlignment AsAlignmentFlags(const Rococo::Sex::SEXML::ISEXMLDirective& directive, cstr attributeName, uint64 defaults)
	{
		auto* a = directive.FindAttributeByName(attributeName);
		if (!a)
		{
			return  static_cast<EGRAlignment>(defaults);
		}

		return static_cast<EGRAlignment>(AsFlags(a->Value(), ParseAlignment));
	}

	struct ToolbarFactory : ISEXMLWidgetFactory
	{
		void Generate(IGreatSexGenerator& generator, const Rococo::Sex::SEXML::ISEXMLDirective& toolbarDirective, Rococo::Gui::IGRWidget& parent) override
		{
			auto& toolbar = Rococo::Gui::CreateToolbar(parent);

			int32 cellPadding = GetOptionalAttribute(toolbarDirective, "Content.Padding.Cell", 2);
			int32 borderPadding = GetOptionalAttribute(toolbarDirective, "Content.Padding.Border", 2);
			EGRAlignment alignment = AsAlignmentFlags(toolbarDirective, "Content.Alignment", (uint64) EGRAlignment::Left | (uint64) EGRAlignment::Top);
			toolbar.SetChildAlignment(alignment, cellPadding, borderPadding);

			generator.SetPanelAttributes(toolbar.Widget(), toolbarDirective);
			generator.GenerateChildren(toolbarDirective, toolbar.Widget());
		}
	};
}