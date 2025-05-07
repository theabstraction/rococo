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

			auto* aCarouselButtonPadding = directive.FindAttributeByName("Carousel.Button.Padding");
			if (aCarouselButtonPadding)
			{
				GuiRect padding = AsGuiRect(aCarouselButtonPadding->Value());
				config.CarouselButtonPadding = GRAnchorPadding{ padding.left, padding.right, padding.top, padding.bottom };
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
				FontQuery fq = generator.GetFont(fontName, aCarouselFont->S());

				Gui::FontSpec spec;
				spec.CharHeight = fq.height;
				spec.FontName = fq.familyName;
				spec.Bold = fq.isBold;
				spec.Italic = fq.isItalic;
				config.CarouselFontId = GetCustodian(owner.Panel()).Fonts().BindFontId(spec);
			}

			auto* aCarouselButtonFont = directive.FindAttributeByName("Carousel.Button.Font");
			if (aCarouselButtonFont)
			{
				cstr fontName = AsString(aCarouselButtonFont->Value()).c_str();
				FontQuery fq = generator.GetFont(fontName, aCarouselButtonFont->S());

				Gui::FontSpec spec;
				spec.CharHeight = fq.height;
				spec.FontName = fq.familyName;
				spec.Bold = fq.isBold;
				spec.Italic = fq.isItalic;
				config.CarouselButtonFontId = GetCustodian(owner.Panel()).Fonts().BindFontId(spec);
			}

			auto* aSliderFont = directive.FindAttributeByName("Slider.Font");
			if (aSliderFont)
			{
				cstr fontName = AsString(aSliderFont->Value()).c_str();
				FontQuery fq = generator.GetFont(fontName, aSliderFont->S());

				Gui::FontSpec spec;
				spec.CharHeight = fq.height;
				spec.FontName = fq.familyName;
				spec.Bold = fq.isBold;
				spec.Italic = fq.isItalic;
				config.SliderFontId = GetCustodian(owner.Panel()).Fonts().BindFontId(spec);
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

			auto* aStringSlotPadding = directive.FindAttributeByName("String.Slot.Padding");
			if (aStringSlotPadding)
			{
				GuiRect padding = AsGuiRect(aStringSlotPadding->Value());
				config.StringSlotPadding = GRAnchorPadding{ padding.left, padding.right, padding.top, padding.bottom };
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

			auto* aSubscribeToCommitButtons = directive.FindAttributeByName("SubscribeToCommitButtons");
			if (aSubscribeToCommitButtons)
			{
				optionWidget.SubscribeToCommitButtons();
			}

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

	struct HintBoxFactory: SEXMLWidgetFactory_AlwaysValid
	{
		void Generate(IGreatSexGenerator& generator, const Rococo::Sex::SEXML::ISEXMLDirective& hintDirective, Rococo::Gui::IGRWidget& parent) override
		{
			auto& hintBox = Rococo::Gui::CreateHintBox(parent);

			Vec2i alignmentSpacing{ 0,0 };

			auto* aTextSpacing = hintDirective.FindAttributeByName("Text.Spacing");
			if (aTextSpacing)
			{
				alignmentSpacing = AsVec2i(aTextSpacing->Value());
			}

			hintBox.SetSpacing(alignmentSpacing);

			auto* aTextAlignment = hintDirective.FindAttributeByName("Text.Alignment");
			if (aTextAlignment)
			{
				cstr sAlign = AsString(aTextAlignment->Value()).c_str();

				try
				{
					GRAlignmentFlags align(sAlign);
					hintBox.SetAlignment(align);
				}
				catch (IException& ex)
				{
					Throw(aTextAlignment->S(), ex.Message());
				}
			}

			auto* aFont = hintDirective.FindAttributeByName("Text.Font");
			if (aFont)
			{
				cstr fontId = AsString(aFont->Value()).c_str();
				auto font = generator.GetFont(fontId, hintDirective.S());
				Gui::FontSpec spec;
				spec.CharHeight = font.height;
				spec.FontName = font.familyName;
				spec.Bold = font.isBold;
				spec.Italic = font.isItalic;
				auto grId = GetCustodian(parent.Panel()).Fonts().BindFontId(spec);
				hintBox.SetFont(grId);
			}

			generator.SetPanelAttributes(hintBox.Widget(), hintDirective);			
		}
	};

	struct RadioButtonsFactory : SEXMLWidgetFactory_AlwaysValid
	{
		void Generate(IGreatSexGenerator& generator, const Rococo::Sex::SEXML::ISEXMLDirective& rbDirective, Rococo::Gui::IGRWidget& parent) override
		{
			auto& radio = Rococo::Gui::CreateRadioButtonsManager(parent);

			Vec2i alignmentSpacing{ 0,0 };

			auto& items = AsStringList(rbDirective["Group"]);	
			for (int i = 0; i < items.NumberOfElements(); i++)
			{
				cstr item = items[i];
				radio.AddButtonToGroup(item);
			}

			cstr navigate = AsString(rbDirective["Navigate"]).c_str();
			if (Eq(navigate, "None"))
			{
				radio.SetNavigation(EGRRadioNavigation::None);
			}
			else if (Eq(navigate, "H") || Eq(navigate, "Horizontal"))
			{
				radio.SetNavigation(EGRRadioNavigation::Horizontal);
			}
			else if (Eq(navigate, "V") || Eq(navigate, "Vertical"))
			{
				radio.SetNavigation(EGRRadioNavigation::Vertical);
			}
			else
			{
				Throw(rbDirective.S(), "Expecting one of None, H, Horizontal, V or Vertical");
			}

			auto& defaultButton = AsString(rbDirective["Default"]);		
			radio.SetDefaultButton(defaultButton.c_str());

			generator.SetPanelAttributes(radio.Widget(), rbDirective);
			generator.GenerateChildren(rbDirective, radio.Widget());
		}
	};

	struct IconFactory : SEXMLWidgetFactory_AlwaysValid
	{
		void Generate(IGreatSexGenerator& generator, const Rococo::Sex::SEXML::ISEXMLDirective& iconDirective, Rococo::Gui::IGRWidget& parent) override
		{
			auto& icon = Rococo::Gui::CreateIcon(parent);

			auto& imagePath = AsString(iconDirective["Image"]);
			icon.SetImagePath(imagePath.c_str());

			auto* presentation = iconDirective.FindAttributeByName("ScaleAgainstFixedHeight");
			if (presentation)
			{
				icon.SetPresentation(EGRIconPresentation::ScaleAgainstFixedHeight);
			}

			auto* imagePadding = iconDirective.FindAttributeByName("Image.Padding");
			if (imagePadding)
			{
				auto rect = AsGuiRect(imagePadding->Value());

				GRAnchorPadding padding{ rect.left, rect.right, rect.top, rect.bottom };
				icon.SetImagePadding(padding);
			}

			generator.SetPanelAttributes(icon.Widget(), iconDirective);
			generator.GenerateChildren(iconDirective, icon.Widget());
		}
	};

	struct ControlPromptFactory : SEXMLWidgetFactory_AlwaysValid
	{
		void Generate(IGreatSexGenerator& generator, const Rococo::Sex::SEXML::ISEXMLDirective& cpDirective, Rococo::Gui::IGRWidget& parent) override
		{
			auto& cp = Rococo::Gui::CreateControlPrompt(parent);

			auto* aFont = cpDirective.FindAttributeByName("Font");
			if (aFont)
			{
				cstr fontId = AsString(aFont->Value()).c_str();
				auto font = generator.GetFont(fontId, cpDirective.S());
				Gui::FontSpec spec;
				spec.CharHeight = font.height;
				spec.FontName = font.familyName;
				spec.Bold = font.isBold;
				spec.Italic = font.isItalic;
				auto grId = GetCustodian(parent.Panel()).Fonts().BindFontId(spec);
				cp.SetFont(grId);
			}

			if (cpDirective.FindAttributeByName("AlignRight"))
			{
				GRAlignmentFlags alignment;
				alignment.Add(EGRAlignment::Right);
				cp.SetAlignment(alignment);
			}

			generator.SetPanelAttributes(cp.Widget(), cpDirective);
			generator.GenerateChildren(cpDirective, cp.Widget());
		}
	};

	struct PromptFactory : ISEXMLWidgetFactory
	{
		void Generate(IGreatSexGenerator& generator, const Rococo::Sex::SEXML::ISEXMLDirective& tabDirective, Rococo::Gui::IGRWidget& parent) override
		{
			UNUSED(generator);

			cstr iconId = AsString(tabDirective["Icon"]).c_str();
			cstr text = AsString(tabDirective["Text"]).c_str();

			auto* cp = Cast<IGRWidgetControlPrompt>(parent);
			if (!cp)
			{
				Throw(tabDirective.S(), "Expecting parent of Prompt to be of type IGRWidgetControlPrompt");
			}

			cp->AddPrompt(iconId, text);
		}

		bool IsValidFrom(const Rococo::Sex::SEXML::ISEXMLDirective& directive) const override
		{
			auto* parent = directive.Parent();
			if (parent == nullptr)
			{
				return false;
			}

			return Eq(parent->FQName(), "ControlPrompt");
		}
	};

	struct ViewportClientFactory : ISEXMLWidgetFactory
	{
		void Generate(IGreatSexGenerator&, const Rococo::Sex::SEXML::ISEXMLDirective& d, Rococo::Gui::IGRWidget&) override
		{
			if (IsValidFrom(d))
			{
				Throw(d.S(), "Viewport.ClientArea may only occur in a Viewport directive");
			}
		}

		bool IsValidFrom(const Rococo::Sex::SEXML::ISEXMLDirective& dir) const override
		{
			return Eq(dir.FQName(), "Viewport");
		}
	};

	struct ViewportOffsetFactory : ISEXMLWidgetFactory
	{
		void Generate(IGreatSexGenerator&, const Rococo::Sex::SEXML::ISEXMLDirective& d, Rococo::Gui::IGRWidget&) override
		{
			if (IsValidFrom(d))
			{
				Throw(d.S(), "Viewport.Offset may only occur in a Viewport directive");
			}
		}

		bool IsValidFrom(const Rococo::Sex::SEXML::ISEXMLDirective& dir) const override
		{
			return Eq(dir.FQName(), "Viewport");
		}
	};

	struct DefIconFactory: ISEXMLWidgetFactory
	{
		void Generate(IGreatSexGenerator & generator, const Rococo::Sex::SEXML::ISEXMLDirective & tabDirective, Rococo::Gui::IGRWidget & parent) override
		{
			UNUSED(generator);

			cstr iconId = AsString(tabDirective["Icon"]).c_str();
			cstr controlType = AsString(tabDirective["For"]).c_str();
			cstr path = AsString(tabDirective["Path"]).c_str();
			
			int vpadding = 0;
			auto* aVPadding = tabDirective.FindAttributeByName("VPadding");
			if (aVPadding)
			{
				vpadding = AsAtomicInt32(aVPadding->Value());
			}

			auto* cp = Cast<IGRWidgetControlPrompt>(parent);
			if (!cp)
			{
				Throw(tabDirective.S(), "Expecting parent of DefIcon to be of type IGRWidgetControlPrompt");
			}

			cp->AddIcon(iconId, controlType, vpadding, path);
		}

		bool IsValidFrom(const Rococo::Sex::SEXML::ISEXMLDirective & directive) const override
		{
			auto* parent = directive.Parent();
			if (parent == nullptr)
			{
				return false;
			}

			return Eq(parent->FQName(), "ControlPrompt");
		}
	};

	struct TabFactory : ISEXMLWidgetFactory
	{
		void Generate(IGreatSexGenerator& generator, const Rococo::Sex::SEXML::ISEXMLDirective& tabDirective, Rococo::Gui::IGRWidget& parent) override
		{
			UNUSED(generator);

			cstr forMeta = AsString(tabDirective["For"]).c_str();
			cstr toggles = AsString(tabDirective["Toggles"]).c_str();

			auto* radio = Cast<IGRWidgetRadioButtons>(parent);
			if (!radio)
			{
				Throw(tabDirective.S(), "Expecting parent of Tab to be of type IGRWidgetRadioButtons");
			}

			radio->AddTab(forMeta, toggles);
		}

		bool IsValidFrom(const Rococo::Sex::SEXML::ISEXMLDirective& directive) const override
		{
			auto* parent = directive.Parent();
			if (parent == nullptr)
			{
				return false;
			}

			return Eq(parent->FQName(), "RadioButtons");
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

	RGBAb AsColour(IGreatSexGenerator& generator, const ISEXMLAttributeValue& v)
	{
		cstr name = AsString(v).c_str();
		RGBAb colour = generator.GetColour(name, GRRenderState(false, false, false), v.S());
		return colour;
	}

	struct GradientFillFactory : SEXMLWidgetFactory_AlwaysValid
	{
		void Generate(IGreatSexGenerator& generator, const Rococo::Sex::SEXML::ISEXMLDirective& directive, Rococo::Gui::IGRWidget& parent) override
		{
			auto& g = Rococo::Gui::CreateGradientFill(parent);

			g.SetTopLeft(AsColour(generator, directive["TopLeft"]));
			g.SetBottomLeft(AsColour(generator, directive["BottomLeft"]));
			g.SetTopRight(AsColour(generator, directive["TopRight"]));
			g.SetBottomRight(AsColour(generator, directive["BottomRight"]));

			cstr fill = AsString(directive["Fill"]).c_str();
			if (EqI(fill, "BANNER"))
			{
				g.Panel().SetFillStyle(EGRFillStyle::BANNER);
			}
			else if (EqI(fill, "SMOOTH"))
			{
				g.Panel().SetFillStyle(EGRFillStyle::SMOOTH);
			}
			else if (EqI(fill, "SOLID") || EqI(fill, "DEFAULT"))
			{
				g.Panel().SetFillStyle(EGRFillStyle::SOLID);
			}
			else
			{
				Throw(directive.S(), "Unknown fill style. Expecting one of BANNER, SMOOTH, SOLID");
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

			generator.SetPanelAttributes(portrait.ClientArea().Widget(), directive);
			generator.GenerateChildren(directive, portrait.ClientArea().Widget());
		}
	};

	EGRRectStyle AsRectStyle(const ISEXMLAttribute& aStyle)
	{
		cstr style = AsString(aStyle.Value()).c_str();

		if (EqI(style, "SHARP"))
		{
			return EGRRectStyle::SHARP;
		}
		else if (EqI(style, "ROUNDED"))
		{
			return EGRRectStyle::ROUNDED;
		}
		else if (EqI(style, "BLUR"))
		{
			return EGRRectStyle::ROUNDED_WITH_BLUR;
		}
		else
		{
			Throw(aStyle.S(), "Expecting either SHARP, BLUR or ROUNDED");
		}
	}

	struct ViewportFactory : SEXMLWidgetFactory_AlwaysValid
	{
		void Generate(IGreatSexGenerator& generator, const Rococo::Sex::SEXML::ISEXMLDirective& viewportDirective, Rococo::Gui::IGRWidget& parent) override
		{
			auto& viewport = Rococo::Gui::CreateViewportWidget(parent);
			viewport.Panel().SetExpandToParentHorizontally().SetExpandToParentVertically();

			auto* aScrollableRectStyle = viewportDirective.FindAttributeByName("Viewport.RectStyle.Scrollable");
			if (aScrollableRectStyle)
			{
				EGRRectStyle style = AsRectStyle(*aScrollableRectStyle);
				viewport.SetClientAreaRectStyleWhenScrollable(style);
			}
			
			auto* aNoScrollableRectStyle = viewportDirective.FindAttributeByName("Viewport.Rectstyle.NotScrollable");
			if (aNoScrollableRectStyle)
			{
				EGRRectStyle style = AsRectStyle(*aNoScrollableRectStyle);
				viewport.SetClientAreaRectStyleWhenNotScrollable(style);
			}

			auto* aSyncDomain = viewportDirective.FindAttributeByName("Viewport.SyncDomainToChildren");
			if (aSyncDomain)
			{
				viewport.SyncDomainToChildren();
			}

			size_t startIndex = 0;
			auto* dClip = SEXML::FindDirective(viewportDirective.Children(), "Viewport.ClientArea", IN OUT startIndex);
			if (dClip)
			{
				generator.SetPanelAttributes(viewport.ClientArea().Widget(), *dClip);
			}

			startIndex = 0;
			auto* dOffset = SEXML::FindDirective(viewportDirective.Children(), "Viewport.Offset", IN OUT startIndex);
			if (dOffset)
			{
				generator.SetPanelAttributes(viewport.ClientArea().Panel().Parent()->Widget(), *dOffset);
			}

			generator.SetPanelAttributes(viewport.Widget(), viewportDirective);
			generator.GenerateChildren(viewportDirective, viewport.ClientArea().Widget());
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

			GRAlignmentFlags alignment;
			alignment.Add(EGRAlignment::HCentre).Add(EGRAlignment::VCentre);

			label.SetAlignment(alignment, { 0,0 });

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

			auto* aMeta = buttonDirective.FindAttributeByName("Meta");
			if (aMeta)
			{
				cstr metaString = AsString(aMeta->Value()).c_str();

				GRControlMetaData meta;
				meta.intData = 0;
				meta.stringData = metaString;
				button.SetMetaData(meta, true);
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

			auto* aClickCriterion = buttonDirective.FindAttributeByName("ClickWhen");
			if (aClickCriterion)
			{
				cstr criterion = AsString(aClickCriterion->Value()).c_str();
				if (EqI(criterion, "DownThenUp"))
				{
					button.SetClickCriterion(EGRClickCriterion::OnDownThenUp);
				}
				else if (EqI(criterion, "Down"))
				{
					button.SetClickCriterion(EGRClickCriterion::OnDown);
				}
				else if (EqI(criterion, "Up"))
				{
					button.SetClickCriterion(EGRClickCriterion::OnUp);
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