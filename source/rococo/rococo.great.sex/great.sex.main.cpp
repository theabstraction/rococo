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

#define MATCH(text, value, numericEquilvalent) if (Strings::EqI(text,value)) return numericEquilvalent;

namespace Rococo::GreatSex
{
	uint64 ParseAnchor(const fstring& item)
	{
		MATCH(item, "left",		0x0000'0001);
		MATCH(item, "top",		0x0000'0002);
		MATCH(item, "right",	0x0000'0004);
		MATCH(item, "bottom",	0x0000'0008);
		MATCH(item, "hexpand",	0x0000'0010);
		MATCH(item, "vexpand",	0x0000'0020);
		Throw(0, "Unknown anchor - %s. Expecting one of { left, top, right, bottom, hexpand, vexpand }", item.buffer);
	}

	void SetPanelAttributes(IGRWidget& widget, const Rococo::Sex::SEXML::ISEXMLDirective& widgetDirective)
	{
		auto& panel = widget.Panel();

		panel.SetAssociatedSExpression(widgetDirective.S());

		Vec2i offset = GetOptionalAttribute(widgetDirective, "Panel.Offset", Vec2i {0,0});
		panel.SetParentOffset(offset);

		Vec2i minimalSpan = GetOptionalAttribute(widgetDirective, "Panel.Span.Min", { 10,10 });
		panel.SetMinimalSpan(minimalSpan);

		Vec2i initialSpan = GetOptionalAttribute(widgetDirective, "Panel.Span", { 10,10 });
		panel.Resize(initialSpan);

		uint64 anchorFlags = AsFlags(widgetDirective["Panel.Anchors"], ParseAnchor);

		fstring desc = GetOptionalAttribute(widgetDirective, "Panel.Description", ""_fstring);
		if (desc.length > 0) panel.SetDesc(desc);

		GRAnchors anchors;
		reinterpret_cast<uint32&>(anchors) = (uint32)anchorFlags;
		panel.Add(anchors);

		bool canFocus = GetOptionalAttribute(widgetDirective, "Panel.CanFocus", false);
		if (canFocus)
		{
			panel.Add(EGRPanelFlags::AcceptsFocus);
		}

		bool tabsCycle = GetOptionalAttribute(widgetDirective, "Panel.TabsCycle", false);
		if (tabsCycle)
		{
			panel.Add(EGRPanelFlags::CycleTabsEndlessly);
		}

		GuiRect padding = GetOptionalAttribute(widgetDirective, "Panel.Padding", { 0,0,0,0 });
		panel.Set(GRAnchorPadding{ padding.left, padding.top, padding.right, padding.bottom });
	}

	void GenerateChildren(IGreatSexGenerator& generator, const Rococo::Sex::SEXML::ISEXMLDirective& widgetDirective, Rococo::Gui::IGRWidget& widget)
	{
		for (size_t i = 0; i < widgetDirective.NumberOfChildren(); i++)
		{
			auto& child = widgetDirective[i];
			generator.AppendWidgetTreeFromSexML(child, widget);
		}
	}

	void OnDivision(IGreatSexGenerator& generator, const Rococo::Sex::SEXML::ISEXMLDirective& divDirective, Rococo::Gui::IGRWidget& parent)
	{
		auto& div = Rococo::Gui::CreateDivision(parent);
		SetPanelAttributes(div, divDirective);
		GenerateChildren(generator, divDirective, div);
	}

	uint8 GetUByteValue(const Rococo::Sex::SEXML::ISEXMLDirective& directive, cstr attributeName)
	{
		int value = SEXML::AsAtomicInt32(directive[attributeName]);
		if (value < 0 || value > 255)
		{
			Throw(directive.S(), "Domain of %s is [0,255]", attributeName);
		}
		return (uint8) value;
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
		{"ColourBackground", EGRSchemeColourSurface::BACKGROUND },
		{"ColourButton", EGRSchemeColourSurface::BUTTON },
		{"ColourButtonEdgeTopLeft", EGRSchemeColourSurface::BUTTON_EDGE_TOP_LEFT },
		{"ColourButtonEdgeBottomRight", EGRSchemeColourSurface::BUTTON_EDGE_BOTTOM_RIGHT },
		{"ColourButtonImageFog", EGRSchemeColourSurface::BUTTON_IMAGE_FOG },

		{"ColourButtonText", EGRSchemeColourSurface::BUTTON_TEXT },

		{"ColourMenuButton", EGRSchemeColourSurface::MENU_BUTTON },
		{"ColourMenuButtonEdgeTopLeft", EGRSchemeColourSurface::MENU_BUTTON_EDGE_TOP_LEFT },
		{"ColourMenuButtonEdgeBottomRight", EGRSchemeColourSurface::MENU_BUTTON_EDGE_BOTTOM_RIGHT },
		{"ColourMenuButtonEdgeBottomRight", EGRSchemeColourSurface::MENU_BUTTON_EDGE_BOTTOM_RIGHT },

		{"ColourContainerBackground", EGRSchemeColourSurface::CONTAINER_BACKGROUND },
		{"ColourContainerTopLeft", EGRSchemeColourSurface::CONTAINER_TOP_LEFT },
		{"ColourContainerBottomRight", EGRSchemeColourSurface::CONTAINER_BOTTOM_RIGHT },

		{"ScrollerButtonBackground", EGRSchemeColourSurface::SCROLLER_BUTTON_BACKGROUND },
		{"ScrollerButtonTopLeft", EGRSchemeColourSurface::SCROLLER_BUTTON_TOP_LEFT },
		{"ScrollerButtonBottomRight", EGRSchemeColourSurface::SCROLLER_BUTTON_BOTTOM_RIGHT },

		{"ScrollerBarBackground", EGRSchemeColourSurface::SCROLLER_BAR_BACKGROUND },
		{"ScrollerBarTopLeft", EGRSchemeColourSurface::SCROLLER_BAR_TOP_LEFT },
		{"ScrollerBarBottomRight", EGRSchemeColourSurface::SCROLLER_BAR_BOTTOM_RIGHT },

		{"ScrollerSliderBackground", EGRSchemeColourSurface::SCROLLER_SLIDER_BACKGROUND },
		{"ScrollerSliderTopLeft", EGRSchemeColourSurface::SCROLLER_SLIDER_TOP_LEFT },
		{"ScrollerSliderBottomRight", EGRSchemeColourSurface::SCROLLER_SLIDER_BOTTOM_RIGHT },

		{"ColourEditor", EGRSchemeColourSurface::EDITOR },
		{"ColourText", EGRSchemeColourSurface::TEXT },
		{"ColourFocus", EGRSchemeColourSurface::FOCUS_RECTANGLE },
		{"ColourEditText", EGRSchemeColourSurface::EDIT_TEXT },

		{"ColourSplitterBackground", EGRSchemeColourSurface::SPLITTER_BACKGROUND },
		{"ColourSplitterEdge", EGRSchemeColourSurface::SPLITTER_EDGE }
	};

	void ApplyToRenderState(const Rococo::Sex::SEXML::ISEXMLDirective& schemeDirective, Gui::GRRenderState state, Rococo::Gui::IGRWidget& widget)
	{
		for (auto& binding : colourDirectiveBindings)
		{
			ApplyColourFromDirective(binding.name, binding.surface, state, widget, schemeDirective);
		}
	}
	
	void OnScheme(IGreatSexGenerator& generator, const Rococo::Sex::SEXML::ISEXMLDirective& schemeDirective, Rococo::Gui::IGRWidget& widget)
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

	void OnVerticalList(IGreatSexGenerator& generator, const Rococo::Sex::SEXML::ISEXMLDirective& verticalListDirective, Rococo::Gui::IGRWidget& parent)
	{
		auto& verticalList = Rococo::Gui::CreateVerticalList(parent);
		SetPanelAttributes(verticalList, verticalListDirective);
		GenerateChildren(generator, verticalListDirective, verticalList);
	}

	void OnTextLabel(IGreatSexGenerator& generator, const Rococo::Sex::SEXML::ISEXMLDirective& textDirective, Rococo::Gui::IGRWidget& parent)
	{
		auto& label = Rococo::Gui::CreateText(parent);

		const fstring text = GetOptionalAttribute(textDirective, "Text", ""_fstring);
		label.SetText(text);

		//!!! Todo - implement label.SetAlignment

		SetPanelAttributes(label.Widget(), textDirective);
		GenerateChildren(generator, textDirective, label.Widget());
	}

	struct GreatSexGenerator : IGreatSexGeneratorSupervisor
	{
		IAllocator& sexmlAllocator;
		AutoFree<ISEXMLRootSupervisor> sexmlParser;
		stringmap<FN_GREAT_SEX_ON_WIDGET> widgetHandlers;

		GreatSexGenerator(IAllocator& _sexmlAllocator): sexmlAllocator(_sexmlAllocator)
		{
			AddHandler("Div", OnDivision);
			AddHandler("Scheme", OnScheme);
		}

		virtual ~GreatSexGenerator()
		{

		}

		void AddHandler(cstr fqName, FN_GREAT_SEX_ON_WIDGET f) override
		{
			auto i = widgetHandlers.find(fqName);
			if (i != widgetHandlers.end())
			{
				Throw(0, "%s: Duplicate fqName: %s", __FUNCTION__, fqName);
			}

			widgetHandlers.insert(fqName, f);
		}

		void Free() override
		{
			auto& allocator = sexmlAllocator;
			this->~GreatSexGenerator();
			allocator.FreeData(this);
		}

		void AppendWidgetTreeFromSexML(const ISEXMLDirective& directive, IGRWidget& branch) override
		{
			cstr fqName = directive.FQName();

			auto i = widgetHandlers.find(fqName);
			if (i == widgetHandlers.end())
			{
				AutoFree<IDynamicStringBuilder> dsb = CreateDynamicStringBuilder(256);
				auto& sb = dsb->Builder();
				sb << "Known directives: ";

				bool first = true;
				for (auto h : widgetHandlers)
				{
					if (!first)
					{
						sb << ", ";
					}
					else
					{
						first = false;
					}

					sb << (cstr) h.first;
				}

				auto knownDirectives = *sb;

				Throw(directive.S(), "Unhandled widget directive: %s.\n%s", fqName, (cstr) knownDirectives);
			}

			auto f = i->second;

			f(*this, directive, branch);
		}

		void AppendWidgetTreeFromSexML(cr_sex s, IGRWidget& branch) override
		{
			sexmlParser = CreateSEXMLParser(sexmlAllocator, s);
			auto& sp = *sexmlParser;

			/* Our Sexml is a list of directives that looks like this:
			*
			* (<widget-type> (<attribute-type-1> <attribute-value-1>) ...  (<attribute-type-N> <attribute-value-N>)
			*     :                 // The colon marks the end of attributes and the beginning of sub-directives
			*     (...child-1...) ...
			*     (...child-N...)
			* )
			*
			* The items are recursive so every child of a widget has the same structure as defined above. Widgets thus form are arbitrarily deep tree.
			*/

			for (int i = 0; i < sp.NumberOfDirectives(); i++)
			{
				auto& widgetDirective = sp[i];
				AppendWidgetTreeFromSexML(widgetDirective, branch);
			}
		}
	};

	ROCOCO_GREAT_SEX_API IGreatSexGeneratorSupervisor* CreateGreatSexGenerator(IAllocator& sexmlAllocator)
	{
		void* pData = sexmlAllocator.Allocate(sizeof GreatSexGenerator);
		return new (pData) GreatSexGenerator(sexmlAllocator);
	}
}