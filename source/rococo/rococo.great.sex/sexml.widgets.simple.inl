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
	struct SEXMLWidgetFactory_AlwaysValid : ISEXMLWidgetFactory
	{
		bool IsValidFrom(const Rococo::Sex::SEXML::ISEXMLDirective&) const override
		{
			return true;
		}
	};

	IGRWidgetMainFrame& GetFrame(IGRWidget& startingPoint, cr_sex src)
	{
		for (auto* panel = &startingPoint.Panel(); panel != nullptr; panel = panel->Parent())
		{
			auto* frame = Cast<IGRWidgetMainFrame>(panel->Widget());
			if (frame != nullptr)
			{
				return *frame;
			}
		}

		Throw(src, "Could not find an IGRWidgetMainFrame in the widget hierarchy");
	}

	struct GameOptionsFactory : ISEXMLWidgetFactory
	{
		ISEXMLGameOptionsList& options;

		GameOptionsFactory(ISEXMLGameOptionsList& _options) : options(_options)
		{

		}

		void Generate(IGreatSexGenerator& generator, const Rococo::Sex::SEXML::ISEXMLDirective& directive, Rococo::Gui::IGRWidget& owner) override
		{
			if (directive.Parent() == nullptr)
			{
				Throw(directive.S(), "GameOptions must not be defined at the root level");
			}

			auto& aGen = directive.GetAttributeByName("Generate");
			cstr key = AsString(aGen.Value()).c_str();

			auto& opt = options.GetOptions(key, aGen.S());

			auto& optionWidget = CreateGameOptionsList(owner, opt);
			generator.SetPanelAttributes(optionWidget.Widget(), directive);

			if (directive.Children().NumberOfDirectives() != 0)
			{
				Throw(directive.S(), "(GameOptions ...) directives do not support child directives");
			}
		}

		bool IsValidFrom(const Rococo::Sex::SEXML::ISEXMLDirective& directive) const override
		{
			return directive.Parent() != nullptr;
		}
	};

	struct InsertFactory : ISEXMLWidgetFactory
	{
		ISEXMLInserter& inserter;
		InsertFactory(ISEXMLInserter& _inserter): inserter(_inserter)
		{

		}

		void Generate(IGreatSexGenerator&, const Rococo::Sex::SEXML::ISEXMLDirective& insertDirective, Rococo::Gui::IGRWidget& owner) override
		{
			if (insertDirective.Parent() != nullptr)
			{
				Throw(insertDirective.S(), "(Insert <filename>) elements must be top-level only");
			}

			auto& path = insertDirective.GetAttributeByName("Path");

			inserter.Insert(AsString(path.Value()).c_str(), insertDirective.S(), owner);
		}

		bool IsValidFrom(const Rococo::Sex::SEXML::ISEXMLDirective& insertDirective) const override
		{
			return insertDirective.Parent() == nullptr;
		}
	};

	struct FrameFactory : ISEXMLWidgetFactory
	{
		void Generate(IGreatSexGenerator& generator, const Rococo::Sex::SEXML::ISEXMLDirective& frameDirective, Rococo::Gui::IGRWidget& parent) override
		{
			if (frameDirective.Parent() != nullptr)
			{
				Throw(frameDirective.S(), "(Frame ..) elements must be top-level only");
			}

			IGRWidgetMainFrame& frame = GetFrame(parent, frameDirective.S());
			generator.SetPanelAttributes(frame.Widget(), frameDirective);
		}

		bool IsValidFrom(const Rococo::Sex::SEXML::ISEXMLDirective& frameDirective) const override
		{
			return frameDirective.Parent() == nullptr;
		}
	};

	struct DivisionFactory : SEXMLWidgetFactory_AlwaysValid
	{
		void Generate(IGreatSexGenerator& generator, const Rococo::Sex::SEXML::ISEXMLDirective& divDirective, Rococo::Gui::IGRWidget& parent) override
		{
			auto& div = Rococo::Gui::CreateDivision(parent);
			generator.SetPanelAttributes(div.Widget(), divDirective);
			generator.GenerateChildren(divDirective, div.Widget());
		}
	};

	struct ViewportFactory : SEXMLWidgetFactory_AlwaysValid
	{
		void Generate(IGreatSexGenerator& generator, const Rococo::Sex::SEXML::ISEXMLDirective& divDirective, Rococo::Gui::IGRWidget& parent) override
		{
			auto& viewport = Rococo::Gui::CreateViewportWidget(parent);
			viewport.Panel().SetExpandToParentHorizontally().SetExpandToParentVertically();
			generator.SetPanelAttributes(viewport.ClientArea().Widget(), divDirective);
			generator.GenerateChildren(divDirective, viewport.ClientArea().Widget());
		}
	};

	struct FrameClientAreaFactory : ISEXMLWidgetFactory
	{
		void Generate(IGreatSexGenerator& generator, const Rococo::Sex::SEXML::ISEXMLDirective& clientAreaDirective, Rococo::Gui::IGRWidget& parent) override
		{
			IGRWidgetMainFrame& frame = GetFrame(parent, clientAreaDirective.S());
			generator.SetPanelAttributes(frame.ClientArea().Widget(), clientAreaDirective);
		}

		bool IsValidFrom(const Rococo::Sex::SEXML::ISEXMLDirective& frameDirective) const override
		{
			return frameDirective.Parent() != nullptr;
		}
	};

	struct VerticalListFactory : SEXMLWidgetFactory_AlwaysValid
	{
		void Generate(IGreatSexGenerator& generator, const Rococo::Sex::SEXML::ISEXMLDirective& verticalListDirective, Rococo::Gui::IGRWidget& parent) override
		{
			auto& verticalList = Rococo::Gui::CreateVerticalList(parent);
			generator.SetPanelAttributes(verticalList.Widget(), verticalListDirective);
			generator.GenerateChildren(verticalListDirective, verticalList.Widget());
		}
	};

	struct TextLabelFactory : SEXMLWidgetFactory_AlwaysValid
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

	struct ButtonFactory : SEXMLWidgetFactory_AlwaysValid
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

	struct ToolbarFactory : SEXMLWidgetFactory_AlwaysValid
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