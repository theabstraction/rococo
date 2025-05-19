#include <rococo.great.sex.h>
#include <sexy.types.h>
#include <Sexy.S-Parser.h>
#include <rococo.sexml.h>
#include <new>
#include <rococo.hashtable.h>
#include <rococo.functional.h>
#include <rococo.strings.h>
#include <rococo.formatting.h>

using namespace Rococo::Gui;
using namespace Rococo::Sex;
using namespace Rococo::Sex::SEXML;
using namespace Rococo::Strings;

namespace Rococo::GreatSex
{
	RGBAb GetColour(const Rococo::Sex::SEXML::ISEXMLDirective& colourDirective);

	uint8 GetColourUByteValue(cstr value, cr_sex S)
	{
		auto iValue = Formatting::TryParseInt32FromDecimalStringSkippingCetera(value);
		if (iValue.code != Formatting::ETryParseResultCode::Success)
		{
			Throw(S, "RGBAb colour components are decimal integers 0 to 255 inclusive");
		}

		if (iValue.Value < 0 || iValue.Value > 255)
		{
			Throw(S, "Domain of a RGBAb colour component is 0 to 255 inclusive");
		}
		return (uint8)iValue.Value;
	}

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

		bool IsValidFrom(const Rococo::Sex::SEXML::ISEXMLDirective& widgetDefinition) const override
		{
			return widgetDefinition.Parent() == nullptr;
		}

		void OnColourSpec(cstr id, const Rococo::Sex::SEXML::ISEXMLDirective& spec)
		{
			auto aFor = spec.FindAttributeByName("For");
			if (!aFor)
			{
				Throw(spec.S(), "Expecting attribute For");
			}

			RGBAb colour;
			GRWidgetRenderState rs(false, false, false);

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

		void OnUniformColourSpec(cstr id, const ISEXMLAttribute& uniformSpec)
		{
			RGBAb colour;
			auto& rgbaList = AsStringList(uniformSpec.Value());
			if (rgbaList.NumberOfElements() != 3 && rgbaList.NumberOfElements() != 4)
			{
				Throw(uniformSpec.S(), "Expecting 3 or 4 elements, each of which is 0-255, representing red, green, blue and (optionally) alpha components in that order.");
			}

			colour.red = GetColourUByteValue(rgbaList[0], uniformSpec.S()[2]);
			colour.green = GetColourUByteValue(rgbaList[1], uniformSpec.S()[3]);
			colour.blue = GetColourUByteValue(rgbaList[2], uniformSpec.S()[4]);	

			if (rgbaList.NumberOfElements() == 3)
			{
				colour.alpha = 255;
			}
			else
			{
				colour.alpha = GetColourUByteValue(rgbaList[3], uniformSpec.S()[5]);
			}

			GRWidgetRenderState::ForEachPermutation([this, id, colour](GRWidgetRenderState rs)
				{
					builder.AddColour(id, colour, rs);
				}
			);
		}

		void Generate(IGreatSexGenerator& generator, const Rococo::Sex::SEXML::ISEXMLDirective& colourDirective, Rococo::Gui::IGRWidget&) override
		{
			UNUSED(generator);

			if (colourDirective.Parent() != nullptr)
			{
				Throw(colourDirective.S(), "Colour directives must occur as top-level directives, never children of other directives");
			}

			auto aId = colourDirective.FindAttributeByName("Id");

			if (aId == nullptr)
			{
				Throw(colourDirective.S(), "Expecting attribute Id");
			}

			auto& sID = AsString(aId->Value());
			cstr id = sID.c_str();

			auto aUniformSpec = colourDirective.FindAttributeByName("Uniform.RGBAb");

			if (aUniformSpec != nullptr)
			{
				OnUniformColourSpec(id, *aUniformSpec);
			}

			for (int j = 0; j < colourDirective.NumberOfChildren(); j++)
			{
				auto& directive = colourDirective[j];
				if (Eq(directive.FQName(), "Spec"))
				{
					OnColourSpec(id, directive);
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
