#include <great.sex.h>
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

		Vec2i offset = GetOptionalAttribute(widgetDirective, "Panel.Offset", Vec2i {0,0});
		panel.SetParentOffset(offset);

		Vec2i minimalSpan = GetOptionalAttribute(widgetDirective, "Panel.Span.Min", { 10,10 });
		panel.SetMinimalSpan(minimalSpan);

		uint64 anchorFlags = AsFlags(widgetDirective["Panel.Anchors"], ParseAnchor);

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
				Throw(directive.S(), "Unhandled widget directive: %s", fqName);
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