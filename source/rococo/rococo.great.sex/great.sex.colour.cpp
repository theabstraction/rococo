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
	RGBAb GetColour(const Rococo::Sex::SEXML::ISEXMLDirective& colourDirective);

	struct ColourFactory : ISEXMLWidgetFactorySupervisor
	{
		ISEXMLColourSchemeBuilder& builder;

		ColourFactory(ISEXMLColourSchemeBuilder& _builder): builder(_builder)
		{

		}

		void Free() override
		{
			delete this;
		}

		void OnColourSpec(cstr id, const Rococo::Sex::SEXML::ISEXMLDirective& spec, Rococo::Gui::IGRWidget&)
		{
			auto aFor = spec.FindAttributeByName("For");
			if (!aFor)
			{
				Throw(spec.S(), "Expecting attribute For");
			}

			RGBAb colour;
			GRRenderState rs(false, false, false);

			auto& renderStateValue = AsStringList(aFor->Value());
			for (size_t i = 0; i < renderStateValue.NumberOfElements(); i++)
			{
				cstr rsString = renderStateValue[i];
				if (Eq(rsString, "pressed") || Eq(rsString, "p"))
				{
					rs.value.bitValues.pressed = 1;
				}
				else if (Eq(rsString, "hovered") || Eq(rsString, "h"))
				{
					rs.value.bitValues.hovered = 1;
				}
				else if (Eq(rsString, "focused") || Eq(rsString, "f"))
				{
					rs.value.bitValues.focused = 1;
				}
				else if (Eq(rsString, "default") || Eq(rsString, "d"))
				{
				}
				else
				{
					Throw(aFor->S()[(int)i + 2], "Expecting one of pressed, hovered, focused, default, p, h, f, d.");
				}
			}

			colour = GetColour(spec);

			builder.AddColour(id, colour, rs);
		}

		void Generate(IGreatSexGenerator& generator, const Rococo::Sex::SEXML::ISEXMLDirective& colourDirective, Rococo::Gui::IGRWidget& widget) override
		{
			UNUSED(generator);

			auto aId = colourDirective.FindAttributeByName("Id");

			if (aId == nullptr)
			{
				Throw(colourDirective.S(), "Expecting attribute Id");
			}

			auto& sID = AsString(aId->Value());
			cstr id = sID.c_str();

			for (int j = 0; j < colourDirective.NumberOfChildren(); j++)
			{
				auto& directive = colourDirective[j];
				if (Eq(directive.FQName(), "Spec"))
				{
					OnColourSpec(id, directive, widget);
				}
				else
				{
					Throw(directive.S(), "Unknown directive, expecting Spec");
				}
			}
		}
	};

	ROCOCO_GREAT_SEX_API ISEXMLWidgetFactorySupervisor* CreateColourHandler(ISEXMLColourSchemeBuilder& builder)
	{
		return new ColourFactory(builder);
	}
}
