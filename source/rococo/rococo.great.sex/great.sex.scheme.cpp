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
using namespace Rococo::Strings;

namespace Rococo::GreatSex
{
	uint8 GetUByteValue(const Rococo::Sex::SEXML::ISEXMLDirective& directive, cstr attributeName)
	{
		int value = SEXML::AsAtomicInt32(directive[attributeName]);
		if (value < 0 || value > 255)
		{
			Throw(directive.S(), "Domain of %s is [0,255]", attributeName);
		}
		return (uint8)value;
	}

	RGBAb GetColour(const Rococo::Sex::SEXML::ISEXMLDirective& colourDirective)
	{
		uint8 red = GetUByteValue(colourDirective, "Red");
		uint8 green = GetUByteValue(colourDirective, "Green");
		uint8 blue = GetUByteValue(colourDirective, "Blue");
		uint8 alpha = GetUByteValue(colourDirective, "Alpha");
		return RGBAb(red, green, blue, alpha);
	}


	bool ApplyColourFromDirective(cstr colourName, EGRSchemeColourSurface surface, Gui::GRRenderState state, Rococo::Gui::IGRWidget& widget, const Rococo::Sex::SEXML::ISEXMLDirective& schemeDirective)
	{
		size_t startIndex = 0;
		auto* colourDirective = schemeDirective.FindFirstChild(REF startIndex, colourName);
		if (colourDirective)
		{
			RGBAb colour = GetColour(*colourDirective);
			widget.Panel().Set(surface, colour, state);
			return true;
		}

		return false;
	}

	const ColourDirectiveBind colourDirectiveBindings[] =
	{
		{"Colour.Background", EGRSchemeColourSurface::BACKGROUND },
		{"Colour.Button", EGRSchemeColourSurface::BUTTON },
		{"Colour.Button.Edge.Top.Left", EGRSchemeColourSurface::BUTTON_EDGE_TOP_LEFT },
		{"Colour.Button.Edge.Bottom.Right", EGRSchemeColourSurface::BUTTON_EDGE_BOTTOM_RIGHT },
		{"Colour.Button.Image.Fog", EGRSchemeColourSurface::BUTTON_IMAGE_FOG },

		{"Colour.Button.Text", EGRSchemeColourSurface::BUTTON_TEXT },

		{"Colour.Menu.Button", EGRSchemeColourSurface::MENU_BUTTON },
		{"Colour.Menu.Button.Edge.Top.Left", EGRSchemeColourSurface::MENU_BUTTON_EDGE_TOP_LEFT },
		{"Colour.Menu.Button.Edge.Bottom.Right", EGRSchemeColourSurface::MENU_BUTTON_EDGE_BOTTOM_RIGHT },
		{"Colour.Menu.Button.Edge.Bottom.Right", EGRSchemeColourSurface::MENU_BUTTON_EDGE_BOTTOM_RIGHT },

		{"Colour.Container.Background", EGRSchemeColourSurface::CONTAINER_BACKGROUND },
		{"Colour.Container.TopLeft", EGRSchemeColourSurface::CONTAINER_TOP_LEFT },
		{"Colour.Container.BottomRight", EGRSchemeColourSurface::CONTAINER_BOTTOM_RIGHT },

		{"Colour.Scroller.Button.Background", EGRSchemeColourSurface::SCROLLER_BUTTON_BACKGROUND },
		{"Colour.Scroller.Button.Top.Left", EGRSchemeColourSurface::SCROLLER_BUTTON_TOP_LEFT },
		{"Colour.Scroller.Button.Bottom.Right", EGRSchemeColourSurface::SCROLLER_BUTTON_BOTTOM_RIGHT },

		{"Colour.Scroller.Bar.Background", EGRSchemeColourSurface::SCROLLER_BAR_BACKGROUND },
		{"Colour.Scroller.Bar.Top.Left", EGRSchemeColourSurface::SCROLLER_BAR_TOP_LEFT },
		{"Colour.Scroller.Bar.Bottom.Right", EGRSchemeColourSurface::SCROLLER_BAR_BOTTOM_RIGHT },

		{"Colour.Scroller.Slider.Background", EGRSchemeColourSurface::SCROLLER_SLIDER_BACKGROUND },
		{"Colour.Scroller.Slider.Top.Left", EGRSchemeColourSurface::SCROLLER_SLIDER_TOP_LEFT },
		{"Colour.Scroller.Slider.Bottom.Right", EGRSchemeColourSurface::SCROLLER_SLIDER_BOTTOM_RIGHT },

		{"Colour.Editor", EGRSchemeColourSurface::EDITOR },
		{"Colour.Text", EGRSchemeColourSurface::TEXT },
		{"Colour.Focus", EGRSchemeColourSurface::FOCUS_RECTANGLE },
		{"Colour.EditText", EGRSchemeColourSurface::EDIT_TEXT },
		{"Colour.Label", EGRSchemeColourSurface::LABEL_BACKGROUND },

		{"Colour.Splitter.Background", EGRSchemeColourSurface::SPLITTER_BACKGROUND },
		{"Colour.Splitter.Edge", EGRSchemeColourSurface::SPLITTER_EDGE },

		{"Colour.Carousel.Text", EGRSchemeColourSurface::CAROUSEL_TEXT },
		{"Colour.Carousel.Background", EGRSchemeColourSurface::CAROUSEL_BACKGROUND },

		{"Colour.GameOption.Background", EGRSchemeColourSurface::GAME_OPTION_BACKGROUND }
	};

	const ColourDirectiveBind* GetColourBindings(OUT size_t& nElements)
	{
		nElements = sizeof(colourDirectiveBindings) / sizeof ColourDirectiveBind;
		return colourDirectiveBindings;
	}

	void ApplyToRenderState(const Rococo::Sex::SEXML::ISEXMLDirective& schemeDirective, Gui::GRRenderState state, Rococo::Gui::IGRWidget& widget)
	{
		for (auto& binding : colourDirectiveBindings)
		{
			ApplyColourFromDirective(binding.name, binding.surface, state, widget, schemeDirective);
		}
	}

	struct OnScheme : ISEXMLWidgetFactorySupervisor
	{
		void Free() override
		{
			delete this;
		}

		bool IsValidFrom(const Rococo::Sex::SEXML::ISEXMLDirective& widgetDefinition) const override
		{
			return widgetDefinition.Parent() == nullptr;
		}

		void Generate(IGreatSexGenerator& generator, const Rococo::Sex::SEXML::ISEXMLDirective& schemeDirective, Rococo::Gui::IGRWidget& widget) override
		{
			UNUSED(generator);

			if (schemeDirective.Parent() != nullptr)
			{
				Throw(schemeDirective.S(), "Scheme directives must be top-level directives, not children of other directives");
			}

			for (int j = 0; j < schemeDirective.NumberOfChildren(); j++)
			{
				auto& directive = schemeDirective[j];
				if (Eq(directive.FQName(), "ApplyTo"))
				{
					Gui::GRRenderState state(0, 0, 0);
					auto& states = SEXML::AsStringList(directive.GetAttributeByName("RenderStates").Value());
					for (int i = 0; i < states.NumberOfElements(); ++i)
					{
						if (Eq(states[i], "focused"_fstring))
						{
							state.value.bitValues.focused = true;
						}
						else if (Eq(states[i], "hovered"_fstring))
						{
							state.value.bitValues.hovered = true;
						}
						else if (Eq(states[i], "pressed"_fstring))
						{
							state.value.bitValues.pressed = true;
						}
						else if (Eq(states[i], "default"_fstring))
						{

						}
						else
						{
							Throw(directive.S(), "Unknown render state: %s", (cstr)states[i]);
						}
					}

					ApplyToRenderState(schemeDirective, state, widget);
				}
				else
				{
					bool foundColour = false;
					for (auto& bind : colourDirectiveBindings)
					{
						if (Eq(bind.name, directive.FQName()))
						{
							// Recognized as a colour directive
							foundColour = true;
							break;
						}
					}

					if (foundColour == false)
					{
						char possibilities[2048];
						StackStringBuilder sb(possibilities, sizeof possibilities);
						sb << "\n\tApplyTo";

						for (auto& bind : colourDirectiveBindings)
						{
							sb << "\n\t" << bind.name;
						}

						Throw(directive.S(), "Unknown directive. Expecting one of %s", possibilities);
					}
				}
			}
		}
	};

	ROCOCO_GREAT_SEX_API ISEXMLWidgetFactorySupervisor* CreateSchemeHandler()
	{
		return new OnScheme();
	}
}
