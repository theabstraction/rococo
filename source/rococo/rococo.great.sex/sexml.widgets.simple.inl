#pragma once
#include <rococo.great.sex.h>
#include <sexy.types.h>
#include <Sexy.S-Parser.h>
#include <rococo.sexml.h>
#include <new>
#include <rococo.hashtable.h>
#include <rococo.functional.h>
#include <rococo.strings.h>
#include <rococo.gui.retained.ex.h>

using namespace Rococo::Gui;
using namespace Rococo::Sex;
using namespace Rococo::Sex::SEXML;
using namespace Rococo::Strings;

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

			GameOptionConfig config;

			auto* aStyle = directive.FindAttributeByName("Title.Style");
			if (aStyle)
			{
				cstr style = AsString(aStyle->Value()).c_str();
				if (Eq(style, "TitlesOnLeft"))
				{
					config.TitlesOnLeft = true;
				}
			}		

			auto* aFont = directive.FindAttributeByName("Title.Font");
			if (aFont)
			{
				cstr fontName = AsString(aFont->Value()).c_str();
				FontQuery fq = generator.GetFont(fontName, aFont->S());

				Gui::FontSpec spec;
				spec.CharHeight = fq.height;
				spec.FontName = fq.familyName;
				spec.Bold = fq.isBold;
				spec.Italic = fq.isItalic;
				config.TitleFontId = GetCustodian(owner.Panel()).Fonts().BindFontId(spec);
			}

			auto* aCarouselFont = directive.FindAttributeByName("Carousel.Font");
			if (aCarouselFont)
			{
				cstr fontName = AsString(aCarouselFont->Value()).c_str();
				FontQuery fq = generator.GetFont(fontName, aFont->S());

				Gui::FontSpec spec;
				spec.CharHeight = fq.height;
				spec.FontName = fq.familyName;
				spec.Bold = fq.isBold;
				spec.Italic = fq.isItalic;
				config.CarouselFontId = GetCustodian(owner.Panel()).Fonts().BindFontId(spec);
			}

			auto* aAlignment = directive.FindAttributeByName("Title.Alignment");
			if (aAlignment)
			{
				cstr sAlign = AsString(aAlignment->Value()).c_str();
				try
				{
					GRAlignmentFlags align(sAlign);
					config.TitleAlignment = align;
				}
				catch (IException& ex)
				{
					Throw(aAlignment->S(), ex.Message());
				}
			}

			auto* aScalarGuageAlignment = directive.FindAttributeByName("Scalar.Guage.Alignment");
			if (aScalarGuageAlignment)
			{
				cstr sAlign = AsString(aScalarGuageAlignment->Value()).c_str();
				try
				{
					GRAlignmentFlags align(sAlign);
					config.ScalarGuageAlignment = align;
				}
				catch (IException& ex)
				{
					Throw(aScalarGuageAlignment->S(), ex.Message());
				}
			}

			auto* aScalarGuageSpacing = directive.FindAttributeByName("Scalar.Guage.Spacing");
			if (aScalarGuageSpacing)
			{
				Vec2i spacing = AsVec2i(aScalarGuageSpacing->Value());
				config.ScalarGuageSpacing = spacing;
			}


			auto* aScalarSlotPadding = directive.FindAttributeByName("Scalar.Slot.Padding");
			if (aScalarSlotPadding)
			{
				GuiRect padding = AsGuiRect(aScalarSlotPadding->Value());
				config.ScalarSlotPadding = GRAnchorPadding{ padding.left, padding.right, padding.top, padding.bottom };
			}

			auto* aHeight = directive.FindAttributeByName("Title.HeightMultiplier");
			if (aHeight)
			{
				double multiplier = AsAtomicDouble(aHeight->Value());
				if (multiplier < 1.0 || multiplier > 10.0)
				{
					Throw(directive.S(), "Valid range 1.0 to 10.0 inclusive");
				}

				config.FontHeightToOptionHeightMultiplier = multiplier;
			}

			auto* aXSpacingMultiplier = directive.FindAttributeByName("Title.XSpacingMultiplier");
			if (aXSpacingMultiplier)
			{
				double multiplier = AsAtomicDouble(aXSpacingMultiplier->Value());
				if (multiplier < 0.0 || multiplier > 10.0)
				{
					Throw(directive.S(), "Valid range 0.0 to 10.0 inclusive");
				}

				config.TitleXSpacingMultiplier = multiplier;
			}

			auto* aCarouselPadding = directive.FindAttributeByName("Carousel.Padding");
			if (aCarouselPadding)
			{
				GuiRect padding = AsGuiRect(aCarouselPadding->Value());
				config.CarouselPadding = GRAnchorPadding{ padding.left, padding.right, padding.top, padding.bottom };
			}

			auto& vKey = directive["Generate"];
			cstr key = AsString(vKey).c_str();

			auto& opt = options.GetOptions(key, vKey.S());

			auto& optionWidget = CreateGameOptionsList(owner, opt, config);
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

			auto& path = insertDirective["Path"];

			inserter.Insert(AsString(path).c_str(), insertDirective.S(), owner);
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

	struct FontFactory : ISEXMLWidgetFactory
	{
		FontFactory()
		{

		}

		void Generate(IGreatSexGenerator& generator, const Rococo::Sex::SEXML::ISEXMLDirective& directive, Rococo::Gui::IGRWidget&) override
		{
			if (!IsValidFrom(directive))
			{
				Throw(directive.S(), "(Font ...) elements must appear at the top level and may not be nested in other directives");
			}

			auto& aId = directive["Id"];
			auto& aFamily = directive["Family"];
			auto& aHeight = directive["Height"];
			auto* mods = directive.FindAttributeByName("Mods");

			bool isBold = false;
			bool isItalic = false;

			if (mods)
			{
				auto& vMods = AsStringList(mods->Value());
				for (size_t i = 0; i < vMods.NumberOfElements(); i++)
				{
					cstr mod = vMods[i];
					if (Eq(mod, "B") || Eq(mod, "Bold"))
					{
						isBold = true;
					}

					if (Eq(mod, "I") || Eq(mod, "Italic"))
					{
						isItalic = true;
					}
				}
			}

			generator.AddFont(AsString(aId).c_str(), AsString(aFamily).c_str(), AsAtomicInt32(aHeight), isBold, isItalic);
		}

		bool IsValidFrom(const Rococo::Sex::SEXML::ISEXMLDirective& directive) const override
		{
			return directive.Parent() == nullptr;
		}
	};

	RGBAb AsColour(IGreatSexGenerator& generator, const ISEXMLAttribute& a)
	{
		cstr name = AsString(a.Value()).c_str();
		RGBAb colour = generator.GetColour(name, GRRenderState(false, false, false), a.S());
		return colour;
	}

	struct GradientFillFactory : SEXMLWidgetFactory_AlwaysValid
	{
		void Generate(IGreatSexGenerator& generator, const Rococo::Sex::SEXML::ISEXMLDirective& directive, Rococo::Gui::IGRWidget& parent) override
		{
			auto& g = Rococo::Gui::CreateGradientFill(parent);

			auto* aColour = directive.FindAttributeByName("TopLeft");
			if (aColour)
			{
				RGBAb colour = AsColour(generator, *aColour);
				g.SetTopLeft(colour);
			}

			aColour = directive.FindAttributeByName("TopRight");
			if (aColour)
			{
				RGBAb colour = AsColour(generator, *aColour);
				g.SetTopRight(colour);
			}

			aColour = directive.FindAttributeByName("BottomLeft");
			if (aColour)
			{
				RGBAb colour = AsColour(generator, *aColour);
				g.SetBottomLeft(colour);
			}

			aColour = directive.FindAttributeByName("BottomRight");
			if (aColour)
			{
				RGBAb colour = AsColour(generator, *aColour);
				g.SetBottomRight(colour);
			}

			generator.SetPanelAttributes(g.Widget(), directive);
			generator.GenerateChildren(directive, g.Widget());
		}
	};

	struct PortraitFactory : SEXMLWidgetFactory_AlwaysValid
	{
		void Generate(IGreatSexGenerator& generator, const Rococo::Sex::SEXML::ISEXMLDirective& directive, Rococo::Gui::IGRWidget& parent) override
		{
			auto& portrait = Rococo::Gui::CreatePortrait(parent);

			auto* aImage = directive.FindAttributeByName("Image");
			if (aImage)
			{
				cstr imagePath = AsString(aImage->Value()).c_str();
				portrait.SetImagePath(imagePath);
			}

			generator.SetPanelAttributes(portrait.Widget(), directive);
			generator.GenerateChildren(directive, portrait.Widget());
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

			auto* aFont = textDirective.FindAttributeByName("Font");
			if (aFont)
			{
				cstr fontId = AsString(aFont->Value()).c_str();
				auto font = generator.GetFont(fontId, textDirective.S());
				Gui::FontSpec spec;
				spec.CharHeight = font.height;
				spec.FontName = font.familyName;
				spec.Bold = font.isBold;
				spec.Italic = font.isItalic;
				auto grId = GetCustodian(parent.Panel()).Fonts().BindFontId(spec);
				label.SetFont(grId);
			}

			auto* aExpand = textDirective.FindAttributeByName("FitH");
			if (aExpand && Eq(AsString(aExpand->Value()).c_str(), "true"))
			{
				label.FitTextH();
			}

			aExpand = textDirective.FindAttributeByName("FitV");
			if (aExpand && Eq(AsString(aExpand->Value()).c_str(), "true"))
			{
				label.FitTextV();
			}

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

			auto* aFont = buttonDirective.FindAttributeByName("Font");
			if (aFont)
			{
				cstr fontId = AsString(aFont->Value()).c_str();
				auto font = generator.GetFont(fontId, buttonDirective.S());
				Gui::FontSpec spec;
				spec.CharHeight = font.height;
				spec.FontName = font.familyName;		
				spec.Bold = font.isBold;
				spec.Italic = font.isItalic;
				auto grId = GetCustodian(parent.Panel()).Fonts().BindFontId(spec);
				button.SetFontId(grId);
			}

			auto* aFitText = buttonDirective.FindAttributeByName("FitTextH");
			if (aFitText)
			{
				button.FitTextHorizontally();
			}

			aFitText = buttonDirective.FindAttributeByName("FitTextV");
			if (aFitText)
			{
				button.FitTextVertically();
			}

			Vec2i alignmentSpacing{ 0,0 };

			auto* aTextSpacing = buttonDirective.FindAttributeByName("Text.Spacing");
			if (aTextSpacing)
			{
				alignmentSpacing = AsVec2i(aTextSpacing->Value());
			}

			auto* aTextAlignment = buttonDirective.FindAttributeByName("Text.Alignment");
			if (aTextAlignment)
			{
				cstr sAlign = AsString(aTextAlignment->Value()).c_str();

				try
				{
					GRAlignmentFlags align(sAlign);
					button.SetAlignment(align, alignmentSpacing);
				}
				catch (IException& ex)
				{
					Throw(aTextAlignment->S(), ex.Message());
				}
			}

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