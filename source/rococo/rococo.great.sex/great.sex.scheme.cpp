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

	struct ColourDirectiveBind
	{
		cstr name;
		EGRSchemeColourSurface surface;
	} colourDirectiveBindings[] =
	{
		{"Background", EGRSchemeColourSurface::BACKGROUND },
		{"Button", EGRSchemeColourSurface::BUTTON },
		{"ButtonEdgeTopLeft", EGRSchemeColourSurface::BUTTON_EDGE_TOP_LEFT },
		{"ButtonEdgeBottomRight", EGRSchemeColourSurface::BUTTON_EDGE_BOTTOM_RIGHT },
		{"ButtonImageFog", EGRSchemeColourSurface::BUTTON_IMAGE_FOG },

		{"ButtonText", EGRSchemeColourSurface::BUTTON_TEXT },

		{"MenuButton", EGRSchemeColourSurface::MENU_BUTTON },
		{"MenuButtonEdgeTopLeft", EGRSchemeColourSurface::MENU_BUTTON_EDGE_TOP_LEFT },
		{"MenuButtonEdgeBottomRight", EGRSchemeColourSurface::MENU_BUTTON_EDGE_BOTTOM_RIGHT },
		{"MenuButtonEdgeBottomRight", EGRSchemeColourSurface::MENU_BUTTON_EDGE_BOTTOM_RIGHT },

		{"ContainerBackground", EGRSchemeColourSurface::CONTAINER_BACKGROUND },
		{"ContainerTopLeft", EGRSchemeColourSurface::CONTAINER_TOP_LEFT },
		{"ContainerBottomRight", EGRSchemeColourSurface::CONTAINER_BOTTOM_RIGHT },

		{"ScrollerButtonBackground", EGRSchemeColourSurface::SCROLLER_BUTTON_BACKGROUND },
		{"ScrollerButtonTopLeft", EGRSchemeColourSurface::SCROLLER_BUTTON_TOP_LEFT },
		{"ScrollerButtonBottomRight", EGRSchemeColourSurface::SCROLLER_BUTTON_BOTTOM_RIGHT },

		{"ScrollerBarBackground", EGRSchemeColourSurface::SCROLLER_BAR_BACKGROUND },
		{"ScrollerBarTopLeft", EGRSchemeColourSurface::SCROLLER_BAR_TOP_LEFT },
		{"ScrollerBarBottomRight", EGRSchemeColourSurface::SCROLLER_BAR_BOTTOM_RIGHT },

		{"ScrollerSliderBackground", EGRSchemeColourSurface::SCROLLER_SLIDER_BACKGROUND },
		{"ScrollerSliderTopLeft", EGRSchemeColourSurface::SCROLLER_SLIDER_TOP_LEFT },
		{"ScrollerSliderBottomRight", EGRSchemeColourSurface::SCROLLER_SLIDER_BOTTOM_RIGHT },

		{"Editor", EGRSchemeColourSurface::EDITOR },
		{"Text", EGRSchemeColourSurface::TEXT },
		{"Focus", EGRSchemeColourSurface::FOCUS_RECTANGLE },
		{"EditText", EGRSchemeColourSurface::EDIT_TEXT },

		{"SplitterBackground", EGRSchemeColourSurface::SPLITTER_BACKGROUND },
		{"SplitterEdge", EGRSchemeColourSurface::SPLITTER_EDGE }
	};

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

		void Generate(IGreatSexGenerator& generator, const Rococo::Sex::SEXML::ISEXMLDirective& schemeDirective, Rococo::Gui::IGRWidget& widget) override
		{
			UNUSED(generator);

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
