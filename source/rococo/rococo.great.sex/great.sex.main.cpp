#include <rococo.great.sex.h>
#include <sexy.types.h>
#include <Sexy.S-Parser.h>
#include <rococo.sexml.h>
#include <new>
#include <rococo.hashtable.h>
#include <rococo.functional.h>
#include <rococo.strings.h>
#include <rococo.io.h>

#define MATCH(text, value, numericEquivalent) if (Strings::EqI(text,value)) return numericEquivalent;

#include "sexml.widgets.simple.inl"

using namespace Rococo;
using namespace Rococo::Gui;
using namespace Rococo::Sex;
using namespace Rococo::Sex::SEXML;
using namespace Rococo::Strings;

namespace Rococo
{
	template<class T>
	cstr AppendKeys(const stringmap<T>& map, StringBuilder& sb, cstr separator = ",")
	{
		bool first = true;

		for (auto& i : map)
		{
			if (first)
			{
				first = false;
			}
			else
			{
				sb << separator;
			}

			sb << (cstr)i.first;
		}

		return (*sb).buffer;
	}
}

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

		struct GreatSexGenerator : IGreatSexGeneratorSupervisor, ISEXMLColourSchemeBuilder, ISEXMLInserter, ISEXMLGameOptionsList
		{
			// Widget Handlers, defined first
			DivisionFactory onDivision;
			AutoFree<ISEXMLWidgetFactorySupervisor> onScheme;
			AutoFree<ISEXMLWidgetFactorySupervisor> onColour;
			VerticalListFactory onVerticalList;
			TextLabelFactory onTextLabel;
			ButtonFactory onButton;
			ToolbarFactory onToolbar;
			FrameFactory onFrame;
			FrameClientAreaFactory onFrameClientArea;
			InsertFactory onInsert;
			GameOptionsFactory onGameOptions;
			ViewportFactory onViewport;
			FontFactory onFont;
			PortraitFactory onPortrait;
			GradientFillFactory onGradientFill;

			Auto<ISParser> insertParser;

			IAllocator& sexmlAllocator;
			
			stringmap<ISEXMLWidgetFactory*> widgetHandlers;

			typedef void(GreatSexGenerator::* MethodForAttribute)(IGRPanel& panel, const ISEXMLAttributeValue& value);

			stringmap<MethodForAttribute> attributeHandlers;

			stringmap<std::vector<ColourBinding>> colourSpecs;

			stringmap<EGRSchemeColourSurface> nameToColourSurface;

			IGreatSexResourceLoader& loader;

			GreatSexGenerator(IAllocator& _sexmlAllocator, IGreatSexResourceLoader& _loader) :
				onScheme(CreateSchemeHandler()),
				onColour(CreateColourHandler(*this)),
				sexmlAllocator(_sexmlAllocator),
				onInsert(*this),
				loader(_loader),
				onGameOptions(*this)
			{
				insertParser = CreateSexParser_2_0(sexmlAllocator);

				AddHandler("Frame", onFrame);
				AddHandler("Frame.ClientArea", onFrameClientArea);
				AddHandler("GameOptions", onGameOptions);
				AddHandler("Colour", *onColour);
				AddHandler("Button", onButton);
				AddHandler("Div", onDivision);
				AddHandler("Scheme", *onScheme);
				AddHandler("VerticalList", onVerticalList);
				AddHandler("Label", onTextLabel);
				AddHandler("Toolbar", onToolbar);
				AddHandler("Insert", onInsert);
				AddHandler("Viewport", onViewport);
				AddHandler("Font", onFont);
				AddHandler("Portrait", onPortrait);
				AddHandler("GradientFill", onGradientFill);

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

			struct GRSexInsertCache
			{
				std::vector<char> buffer;
				Auto<ISourceCode> proxy;
			};

			stringmap<GRSexInsertCache> inserts;

			void OnInsertLoaded(cr_sex src, GRSexInsertCache& cache, IGRWidget& owner)
			{
				Auto<ISParserTree> tree;

				try
				{
					tree = insertParser->CreateTree(*cache.proxy);
				}
				catch (ParseException& ex)
				{
					char message[1024];
					SafeFormat(message, "Error loading sexml from directive at line %d in %s: %s", src.Start().y, src.Tree().Source().Name(), ex.Message());
					ParseException deepEx(ex.Start(), ex.End(), ex.Name(), message, ex.Specimen(), ex.Source());
					throw deepEx;
				}
				catch (IException& ex)
				{
					char message[1024];
					SafeFormat(message, "Error loading sexml from directive at line %d in %s: %s", src.Start().y, src.Tree().Source().Name(), ex.Message());
					ParseException deepEx(src.Start(), src.End(), src.Tree().Source().Name(), message, "", &src);
					throw deepEx;
				}

				try
				{
					AppendWidgetTreeFromSexML(tree->Root(), owner);
				}
				catch (ParseException& ex)
				{
					char message[1024];
					SafeFormat(message, "Error loading sexml from directive at line %d in %s: %s", src.Start().y, src.Tree().Source().Name(), ex.Message());
					ParseException deepEx(ex.Start(), ex.End(), ex.Name(), message, ex.Specimen(), ex.Source());
					throw deepEx;
				}
				catch (IException& ex)
				{
					char message[1024];
					SafeFormat(message, "Error loading sexml from directive at line %d in %s: %s", src.Start().y, src.Tree().Source().Name(), ex.Message());
					ParseException deepEx(tree->Root().Start(), tree->Root().End(), cache.proxy->Name(), message, "", &src);
					throw deepEx;
				}
			}

			void Insert(cstr filePath, cr_sex src, IGRWidget& owner) override
			{
				if (inserts.find(filePath) != inserts.end())
				{
					Throw(src, "Duplicate insert of %s", filePath);
				}

				struct : Rococo::IO::ILoadEventsCallback
				{
					cstr filePath = nullptr;
					GreatSexGenerator* This = nullptr;
					GRSexInsertCache* cache = nullptr;
	
					void OnFileOpen(int64 fileLength) override
					{
						if (fileLength == 0)
						{
							Throw(0, "%s: file empty", filePath);
						}

						if (fileLength > (int64) 1_megabytes)
						{
							Throw(0, "%s: max length > 1 MB", filePath);
						}

						auto i = This->inserts.insert(filePath, GRSexInsertCache()).first;
						cache = &i->second;
						cache->buffer.resize(fileLength + 1);
					}

					void OnDataAvailable(IO::ILoadEventReader& reader) override
					{
						uint32 bytesRead;
						uint32 nBytesToRead = (uint32)(cache->buffer.size() - 1);
						reader.ReadData(&cache->buffer[0], nBytesToRead, OUT bytesRead);
						cache->buffer[cache->buffer.size()-1] = 0;
					}
				} onLoad;

				onLoad.filePath = filePath;
				onLoad.This = this;

				loader.LoadGreatSexResource(filePath, onLoad);

				cstr buffer = onLoad.cache->buffer.data();
				onLoad.cache->proxy = insertParser->ProxySourceBuffer(buffer, (int) strlen(buffer), { 0,0 }, filePath, nullptr);

				OnInsertLoaded(src, *onLoad.cache, owner);
			}

			stringmap<Game::Options::IGameOptions*> mapNameToOptions;

			void AddOptions(Game::Options::IGameOptions& options, cstr key) override
			{
				auto i = mapNameToOptions.insert(key, &options);
				if (!i.second)
				{
					Throw(0, "Duplicate key '%s': %s", key, __FUNCTION__);
				}
			}

			Game::Options::IGameOptions& GetOptions(cstr key, cr_sex src) override
			{
				auto i = mapNameToOptions.find(key);
				if (i != mapNameToOptions.end())
				{
					return *i->second;
				}

				if (mapNameToOptions.empty())
				{
					Throw(src, "Could not find option '%s'. No known options. C++ developer should use IGreatSexGenerator::AddOptions(...)", key);
				}

				AutoFree<IDynamicStringBuilder> dsb = CreateDynamicStringBuilder(256);
				Throw(src, "Could not find option '%s'. Known options: %s", key, AppendKeys(mapNameToOptions, dsb->Builder()));
			}

			struct FontDef
			{
				HString id;
				HString family;
				int height = 11;
				bool isBold = false;
				bool isItalic = false;
			};

			stringmap<FontDef> fonts;

			void AddFont(cstr id, cstr family, int height, bool isBold, bool isItalic) override
			{
				FontDef def;
				def.id = id;
				def.family = family;
				def.height = height;
				def.isBold = isBold;
				def.isItalic = isItalic;
				fonts[id] = def;
			}

			FontQuery GetFont(cstr id, const Sex::ISExpression& s) const override
			{
				auto i = fonts.find(id);
				if (i == fonts.end())
				{
					char err[4096];
					StackStringBuilder sb(err, sizeof err);
					sb << "Unknown Font " << id << ". Known fonts :";

					int count = 0;
					for (auto h : fonts)
					{
						if (count > 0)
						{
							sb << ", ";
						}

						sb << (cstr)h.first;
						count++;
					}

					Throw(s, "%s", err);
				}

				FontQuery q;
				q.id = i->first;
				q.height = i->second.height;
				q.familyName = i->second.family;
				q.isBold = i->second.isBold;
				q.isItalic = i->second.isItalic;
				return q;
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
						if (!h.second->IsValidFrom(directive))
						{
							continue;
						}

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

					cstr knownDirectives = *sb;

					Throw(directive.S(), "Unhandled widget directive: %s.\n%s", fqName, knownDirectives);
				}

				auto factory = i->second;
				factory->Generate(*this, directive, branch);
			}

			void AppendWidgetTreeFromSexML(cr_sex s, IGRWidget& branch) override
			{
				AutoFree<ISEXMLRootSupervisor> sexmlParser = CreateSEXMLParser(sexmlAllocator, s);
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
				cstr raw = AsString(value).c_str();
				if (EndsWith(raw, "%"))
				{
					char intBuffer[16];
					SafeFormat(intBuffer, "%s", raw);
					int height = atoi(intBuffer);
					panel.SetConstantHeight(height, true);
				}
				else
				{
					int height = SEXML::AsAtomicInt32(value);
					panel.SetConstantHeight(height);
				}
			}

			void OnAttribute_FixedWidth(IGRPanel& panel, const ISEXMLAttributeValue& value)
			{
				cstr raw = AsString(value).c_str();
				if (EndsWith(raw, "%"))
				{
					char intBuffer[16];
					SafeFormat(intBuffer, "%s", raw);
					int width = atoi(intBuffer);
					panel.SetConstantWidth(width, true);
				}
				else
				{
					int width = SEXML::AsAtomicInt32(value);
					panel.SetConstantWidth(width);
				}
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

			void OnAttribute_ChildPadding(IGRPanel& panel, const ISEXMLAttributeValue& value)
			{
				int padding = AsAtomicInt32(value);
				panel.SetChildPadding(padding);
			}

			void OnAttribute_Fit(IGRPanel& panel, const ISEXMLAttributeValue& value)
			{
				auto& fit = AsStringList(value);
				for (size_t i = 0; i < fit.NumberOfElements(); ++i)
				{
					cstr f = fit[i];
					if (Eq(f, "Horizontal") || Eq(f, "H"))
					{
						panel.SetFitChildrenHorizontally();
					}
					else if (Eq(f, "Vertical") || Eq(f, "V"))
					{
						panel.SetFitChildrenVertically();
					}
				}
			}

			void OnAttribute_RectStyle(IGRPanel& panel, const ISEXMLAttributeValue& value)
			{
				cstr style = AsString(value).c_str();

				if (EqI(style, "SHARP"))
				{
					panel.SetRectStyle(EGRRectStyle::SHARP);
				}
				else if (EqI(style, "ROUNDED"))
				{
					panel.SetRectStyle(EGRRectStyle::ROUNDED);
				}
				else
				{
					Throw(value.S(), "Expecting either SHARP or ROUNDED");
				}
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

			void OnAttribute_ExpandH(IGRPanel& panel, const ISEXMLAttributeValue&)
			{
				panel.SetExpandToParentHorizontally();
			}

			void OnAttribute_ExpandV(IGRPanel& panel, const ISEXMLAttributeValue&)
			{
				panel.SetExpandToParentVertically();
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

			RGBAb GetColour(cstr key, GRRenderState rs, cr_sex s) override
			{
				auto spec = colourSpecs.find(key);
				if (spec == colourSpecs.end())
				{
					Throw(s, "Could not find colour '%s'. Ensure a (Colour (Id %s)...) definition is in the root directives", key, key);
				}

				for (auto& colourSpec : spec->second)
				{
					if (colourSpec.rs == rs)
					{
						return colourSpec.colour;
					}
				}

				return RGBAb(255, 0, 0, 255);
			}

			void OnAttributePrefix_Colour(IGRPanel& panel, cstr name, const ISEXMLAttributeStringValue& value)
			{
				auto i = nameToColourSurface.find(name);
				if (i == nameToColourSurface.end())
				{
					AutoFree<IDynamicStringBuilder> dsb = CreateDynamicStringBuilder(256);
					Throw(value.S(), "No such colour surface: %s. Known:\t\n%s", name, AppendKeys(nameToColourSurface, dsb->Builder(), "\t\n"));
				}

				cstr key = value.c_str();

				auto spec = colourSpecs.find(key);
				if (spec == colourSpecs.end())
				{
					Throw(value.S(), "Could not find colour '%s'. Ensure a (Colour (Id %s)...) definition is in the root directives", key, key);
				}

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
					attributeHandlers["Panel.ExpandH"] = &GreatSexGenerator::OnAttribute_ExpandH;
					attributeHandlers["Panel.ExpandV"] = &GreatSexGenerator::OnAttribute_ExpandV;
					attributeHandlers["Panel.ChildPadding"] = &GreatSexGenerator::OnAttribute_ChildPadding;
					attributeHandlers["Panel.Fit"] = &GreatSexGenerator::OnAttribute_Fit;
					attributeHandlers["Panel.RectStyle"] = &GreatSexGenerator::OnAttribute_RectStyle;
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

	ROCOCO_GREAT_SEX_API IGreatSexGeneratorSupervisor* CreateGreatSexGenerator(IAllocator& sexmlAllocator, IGreatSexResourceLoader& loader)
	{
		void* pData = sexmlAllocator.Allocate(sizeof Implementation::GreatSexGenerator);
		return new (pData) Implementation::GreatSexGenerator(sexmlAllocator, loader);
	}
}