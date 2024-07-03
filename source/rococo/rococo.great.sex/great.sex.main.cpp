#include <rococo.great.sex.h>
#include <sexy.types.h>
#include <Sexy.S-Parser.h>
#include <rococo.sexml.h>
#include <new>
#include <rococo.hashtable.h>
#include <rococo.functional.h>
#include <rococo.strings.h>

#include "sexml.widgets.simple.inl"

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

	struct GreatSexGenerator : IGreatSexGeneratorSupervisor
	{
		// Widget Handlers, defined first
		OnDivision onDivision;
		AutoFree<ISEXMLWidgetFactorySupervisor> onScheme;
		OnVerticalList onVerticalList;

		IAllocator& sexmlAllocator;
		AutoFree<ISEXMLRootSupervisor> sexmlParser;
		stringmap<ISEXMLWidgetFactory*> widgetHandlers;

		typedef void(GreatSexGenerator::* MethodForAttribute)(IGRPanel& panel, const ISEXMLAttributeValue& value);

		stringmap<MethodForAttribute> attributeHandlers;

		GreatSexGenerator(IAllocator& _sexmlAllocator): onScheme(CreateSchemeHandler()), sexmlAllocator(_sexmlAllocator)
		{
			AddHandler("Div", onDivision);
			AddHandler("Scheme", *onScheme);
			AddHandler("VerticalList", onVerticalList);
		}

		virtual ~GreatSexGenerator()
		{

		}

		void AddHandler(cstr fqName, ISEXMLWidgetFactory& f) override
		{
			auto i = widgetHandlers.find(fqName);
			if (i != widgetHandlers.end())
			{
				Throw(0, "%s: Duplicate fqName: %s", __FUNCTION__, fqName);
			}

			widgetHandlers.insert(fqName, &f);
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

			auto factory = i->second;

			factory->Generate(*this, directive, branch);
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

		void GenerateChildren(const ISEXMLDirective& widgetDirective, Rococo::Gui::IGRWidget& widget) override
		{
			for (size_t i = 0; i < widgetDirective.NumberOfChildren(); i++)
			{
				auto& child = widgetDirective[i];
				AppendWidgetTreeFromSexML(child, widget);
			}
		}

		void OnAttribute_Offset(IGRPanel& panel, const ISEXMLAttributeValue& value)
		{
			Vec2i offset = SEXML::AsVec2i(value);
			panel.SetParentOffset(offset);
		}

		void OnAttribute_Span(IGRPanel& panel, const ISEXMLAttributeValue& value)
		{
			Vec2i initialSpan = SEXML::AsVec2i(value);
			panel.Resize(initialSpan);
		}

		void OnAttribute_SpanMin(IGRPanel& panel, const ISEXMLAttributeValue& value)
		{
			Vec2i minimalSpan = SEXML::AsVec2i(value);
			panel.SetMinimalSpan(minimalSpan);
		}

		void OnAttribute_Anchors(IGRPanel& panel, const ISEXMLAttributeValue& value)
		{
			uint64 anchorFlags = AsFlags(value, ParseAnchor);
			GRAnchors anchors;
			reinterpret_cast<uint32&>(anchors) = (uint32)anchorFlags;
			panel.Add(anchors);
		}

		void OnAttribute_Description(IGRPanel& panel, const ISEXMLAttributeValue& value)
		{
			fstring desc = AsString(value).ToFString();
			if (desc.length > 0) panel.SetDesc(desc);
		}

		void OnAttribute_CanFocus(IGRPanel& panel, const ISEXMLAttributeValue& value)
		{
			bool canFocus = AsBool(value);
			if (canFocus)
			{
				panel.Add(EGRPanelFlags::AcceptsFocus);
			}
		}

		void OnAttribute_TabsCycle(IGRPanel& panel, const ISEXMLAttributeValue& value)
		{
			bool canFocus = AsBool(value);
			if (canFocus)
			{
				panel.Add(EGRPanelFlags::AcceptsFocus);
			}
		}

		void OnAttribute_Padding(IGRPanel& panel, const ISEXMLAttributeValue& value)
		{
			GuiRect padding = AsGuiRect(value);
			panel.Set(GRAnchorPadding{ padding.left, padding.top, padding.right, padding.bottom });
		}

		void SetPanelAttributes(IGRWidget& widget, const ISEXMLDirective& widgetDirective) override
		{
			auto& panel = widget.Panel();
			panel.SetAssociatedSExpression(widgetDirective.S());

			if (attributeHandlers.empty())
			{
				attributeHandlers["Panel.Offset"] = &GreatSexGenerator::OnAttribute_Offset;
				attributeHandlers["Panel.Span"] = &GreatSexGenerator::OnAttribute_Span;
				attributeHandlers["Panel.Span.Min"] = &GreatSexGenerator::OnAttribute_SpanMin;
				attributeHandlers["Panel.Anchors"] = &GreatSexGenerator::OnAttribute_Anchors;
				attributeHandlers["Panel.Description"] = &GreatSexGenerator::OnAttribute_Description;
				attributeHandlers["Panel.CanFocus"] = &GreatSexGenerator::OnAttribute_CanFocus;
				attributeHandlers["Panel.TabsCycle"] = &GreatSexGenerator::OnAttribute_TabsCycle;
				attributeHandlers["Panel.Padding"] = &GreatSexGenerator::OnAttribute_Padding;
			}

			for (size_t i = 0; i < widgetDirective.NumberOfAttributes(); i++)
			{
				auto& a = widgetDirective.GetAttributeByIndex(i);
				cstr name = a.Name();

				auto attributeMethod = attributeHandlers.find(name);
				if (attributeMethod != attributeHandlers.end())
				{
					auto method = attributeMethod->second;
					(this->*method)(panel, a.Value());
				}
				else
				{
					char err[4096];
					StackStringBuilder sb(err, sizeof err);
					sb << "Unknown attribute " << name << ". Known attributes :";

					int count = 0;
					for (auto h : attributeHandlers)
					{
						if (count > 0)
						{
							sb << ", ";
						}

						sb << (cstr)h.first;
						count++;
					}

					Throw(a.S(), "%s", err);
				}
			}
		}
	};

	ROCOCO_GREAT_SEX_API IGreatSexGeneratorSupervisor* CreateGreatSexGenerator(IAllocator& sexmlAllocator)
	{
		void* pData = sexmlAllocator.Allocate(sizeof GreatSexGenerator);
		return new (pData) GreatSexGenerator(sexmlAllocator);
	}
}