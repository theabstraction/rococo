#include <rococo.great.sex.h>
#include <sexy.types.h>
#include <Sexy.S-Parser.h>
#include <rococo.sexml.h>
#include <new>
#include <rococo.hashtable.h>
#include <rococo.functional.h>
#include <rococo.strings.h>

#define MATCH(text, value, numericEquilvalent) if (Strings::EqI(text,value)) return numericEquilvalent;

#include "sexml.widgets.simple.inl"

using namespace Rococo::Gui;
using namespace Rococo::Sex;
using namespace Rococo::Sex::SEXML;
using namespace Rococo::Strings;

namespace Rococo::GreatSex
{
	const ColourDirectiveBind* GetColourBindings(OUT size_t& nElements);

	namespace Implementation
	{
		struct ColourBinding
		{
			GRRenderState rs;
			RGBAb colour;
		};

		struct GreatSexGenerator : IGreatSexGeneratorSupervisor, ISEXMLColourSchemeBuilder
		{
			// Widget Handlers, defined first
			DivisionFactory onDivision;
			AutoFree<ISEXMLWidgetFactorySupervisor> onScheme;
			AutoFree<ISEXMLWidgetFactorySupervisor> onColour;
			VerticalListFactory onVerticalList;
			TextLabelFactory onTextLabel;
			ButtonFactory onButton;
			ToolbarFactory onToolbar;

			IAllocator& sexmlAllocator;
			AutoFree<ISEXMLRootSupervisor> sexmlParser;
			stringmap<ISEXMLWidgetFactory*> widgetHandlers;

			typedef void(GreatSexGenerator::* MethodForAttribute)(IGRPanel& panel, const ISEXMLAttributeValue& value);

			stringmap<MethodForAttribute> attributeHandlers;

			stringmap<std::vector<ColourBinding>> colourSpecs;

			stringmap<EGRSchemeColourSurface> nameToColourSurface;

			GreatSexGenerator(IAllocator& _sexmlAllocator) :
				onScheme(CreateSchemeHandler()),
				onColour(CreateColourHandler(*this)),
				sexmlAllocator(_sexmlAllocator)
			{
				AddHandler("Colour", *onColour);
				AddHandler("Button", onButton);
				AddHandler("Div", onDivision);
				AddHandler("Scheme", *onScheme);
				AddHandler("VerticalList", onVerticalList);
				AddHandler("Label", onTextLabel);
				AddHandler("Toolbar", onToolbar);

				size_t nElements;
				const ColourDirectiveBind* bindings = GetColourBindings(OUT nElements);

				for (size_t i = 0; i < nElements; i++)
				{
					nameToColourSurface.insert(bindings[i].name, bindings[i].surface);
				}
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

						sb << (cstr)h.first;
					}

					auto knownDirectives = *sb;

					Throw(directive.S(), "Unhandled widget directive: %s.\n%s", fqName, (cstr)knownDirectives);
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
				panel.SetConstantSpan(initialSpan);
			}

			void OnAttribute_FixedHeight(IGRPanel& panel, const ISEXMLAttributeValue& value)
			{
				int height = SEXML::AsAtomicInt32(value);
				panel.SetConstantHeight(height);
			}

			void OnAttribute_FixedWidth(IGRPanel& panel, const ISEXMLAttributeValue& value)
			{
				int width = SEXML::AsAtomicInt32(value);
				panel.SetConstantWidth(width);
			}

			void OnAttribute_SpanMin(IGRPanel& panel, const ISEXMLAttributeValue& value)
			{
				Vec2i minimalSpan = SEXML::AsVec2i(value);
				panel.SetMinimalSpan(minimalSpan);
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
				panel.Set(GRAnchorPadding{ padding.left, padding.right, padding.top, padding.bottom });
			}

			void ParseExpansion(IGRPanel& panel, cstr item, cr_sex source)
			{
				if (Eq(item, "Horizontal") || Eq(item, "H"))
				{
					panel.SetExpandToParentHorizontally();
				}
				else if (Eq(item, "Vertical") || Eq(item, "V"))
				{
					panel.SetExpandToParentVertically();
				}
				else
				{
					Throw(source, "Unknown expansion argument. Expecting one of H, V, Horizontal, Vertical");
				}
			}

			void OnAttribute_Expand(IGRPanel& panel, const ISEXMLAttributeValue& value)
			{
				if (value.S().NumberOfElements() == 2)
				{
					ParseExpansion(panel, AsString(value).c_str(), value.S());
				}
				else
				{
					auto& list = AsStringList(value);
					size_t nElements = list.NumberOfElements();
					for (size_t i = 0; i < nElements; i++)
					{
						cstr item = list[i];
						ParseExpansion(panel, item, list.S()[(int) i + 2]);
					}
				}
			}

			void OnAttribute_Layout(IGRPanel& panel, const ISEXMLAttributeValue& value)
			{
				auto& avLayout = AsString(value);
				cstr sLayout = avLayout.c_str();

				ELayoutDirection layout = ELayoutDirection::None;
				
				if (Eq(sLayout, "None"))
				{
				}
				else if (Eq(sLayout, "LeftToRight"))
				{
					layout = ELayoutDirection::LeftToRight;
				}
				else if (Eq(sLayout, "TopToBottom"))
				{
					layout = ELayoutDirection::TopToBottom;
				}
				else if (Eq(sLayout, "RightToLeft"))
				{
					layout = ELayoutDirection::RightToLeft;
				}
				else if (Eq(sLayout, "BottomToTop"))
				{
					layout = ELayoutDirection::BottomToTop;
				}
				else
				{
					Throw(value.S(), "Could not interpret %s as a layout. Permitted values are: None, LeftToRight, TopToBottom, RightToLeft, BottomToTop", sLayout);
				}

				panel.SetLayoutDirection(layout);
			}

			void OnAttributePrefix_Colour(IGRPanel& panel, cstr name, const ISEXMLAttributeStringValue& value)
			{
				auto i = nameToColourSurface.find(name);
				if (i == nameToColourSurface.end())
				{
					Throw(value.S(), "No such colour surface: %s", name);
				}

				auto spec = colourSpecs.find(value.c_str());
				for (auto& colourSpec : spec->second)
				{
					panel.Set(i->second, colourSpec.colour, colourSpec.rs);
				}
			}

			void SetPanelAttributes(IGRWidget& widget, const ISEXMLDirective& widgetDirective) override
			{
				auto& panel = widget.Panel();
				panel.SetAssociatedSExpression(widgetDirective.S());

				if (attributeHandlers.empty())
				{
					attributeHandlers["Panel.Offset"] = &GreatSexGenerator::OnAttribute_Offset;
					attributeHandlers["Panel.Span"] = &GreatSexGenerator::OnAttribute_Span;
					attributeHandlers["Panel.FixedWidth"] = &GreatSexGenerator::OnAttribute_FixedWidth;
					attributeHandlers["Panel.FixedHeight"] = &GreatSexGenerator::OnAttribute_FixedHeight;
					attributeHandlers["Panel.Span.Min"] = &GreatSexGenerator::OnAttribute_SpanMin;
					attributeHandlers["Panel.Description"] = &GreatSexGenerator::OnAttribute_Description;
					attributeHandlers["Panel.CanFocus"] = &GreatSexGenerator::OnAttribute_CanFocus;
					attributeHandlers["Panel.TabsCycle"] = &GreatSexGenerator::OnAttribute_TabsCycle;
					attributeHandlers["Panel.Padding"] = &GreatSexGenerator::OnAttribute_Padding;
					attributeHandlers["Panel.Layout"] = &GreatSexGenerator::OnAttribute_Layout;
					attributeHandlers["Panel.Expand"] = &GreatSexGenerator::OnAttribute_Expand;
				}

				for (size_t i = 0; i < widgetDirective.NumberOfAttributes(); i++)
				{
					auto& a = widgetDirective.GetAttributeByIndex(i);
					cstr name = a.Name();

					if (StartsWith(name, "Colour."))
					{
						OnAttributePrefix_Colour(panel, name, AsString(a.Value()));
					}

					if (!StartsWith(name, "Panel."))
					{
						continue;
					}

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
						sb << "Unknown Panel attribute " << name << ". Known attributes :";

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

			void AddColour(cstr id, RGBAb colour, GRRenderState rs) override
			{
				auto i = colourSpecs.find(id);
				if (i == colourSpecs.end())
				{
					i = colourSpecs.insert(id, std::vector<ColourBinding>()).first;
				}

				i->second.push_back({ rs, colour });
			}
		};
	}

	ROCOCO_GREAT_SEX_API IGreatSexGeneratorSupervisor* CreateGreatSexGenerator(IAllocator& sexmlAllocator)
	{
		void* pData = sexmlAllocator.Allocate(sizeof Implementation::GreatSexGenerator);
		return new (pData) Implementation::GreatSexGenerator(sexmlAllocator);
	}
}