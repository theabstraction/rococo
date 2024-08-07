#include <rococo.gui.retained.ex.h>
#include <rococo.maths.i32.h>
#include <string>
#include <rococo.reflector.h>
#include <rococo.strings.h>
#include <vector>

using namespace Rococo;
using namespace Rococo::Gui;
using namespace Rococo::Reflection;
using namespace Rococo::Strings;

namespace GRANON
{
	struct PreviewData;

	enum class PrimitiveType
	{
		I32, I64, U64, F32, F64, BOOL, CSTR, SUB_OBJECT
	};

	union PreviewPrimitive
	{
		int32 int32Value;
		int64 int64Value;
		uint64 uint64Value;
		float float32Value;
		double float64Value;
		bool boolValue;
		PreviewData* pSubObject;
	};

	struct EditableString
	{
		HString text;
		int32 capacity;
	};

	struct PrimitiveVariant
	{
		EditableString stringValue;
		PreviewPrimitive primitive;
		PrimitiveType type;
	};

	void Assign(PrimitiveVariant& v, int32 value)
	{
		v.primitive.int32Value = value;
		v.type = PrimitiveType::I32;
	}

	void Assign(PrimitiveVariant& v, int64 value)
	{
		v.primitive.int64Value = value;
		v.type = PrimitiveType::I64;
	}

	void Assign(PrimitiveVariant& v, uint64 value)
	{
		v.primitive.uint64Value = value;
		v.type = PrimitiveType::U64;
	}

	void Assign(PrimitiveVariant& v, float value)
	{
		v.primitive.float32Value = value;
		v.type = PrimitiveType::F32;
	}

	void Assign(PrimitiveVariant& v, double value)
	{
		v.primitive.float64Value = value;
		v.type = PrimitiveType::F64;
	}

	void Assign(PrimitiveVariant& v, bool value)
	{
		v.primitive.boolValue = value;
		v.type = PrimitiveType::BOOL;
	}

	void Assign(PrimitiveVariant& v, cstr value, int32 capacity)
	{
		v.primitive.float64Value = 0;
		v.stringValue.text = value;
		v.stringValue.capacity = capacity;
		v.type = PrimitiveType::CSTR;
	}

	void Assign(PrimitiveVariant& v, PreviewData* subObject)
	{
		v.primitive.pSubObject = subObject;
		v.type = PrimitiveType::SUB_OBJECT;
	}

	struct PreviewField
	{
		HString fieldName;
		PrimitiveVariant value;
	};

	void ToAscii(const PrimitiveVariant& variant, char* buffer, size_t capacity, int32 radix = 10)
	{
		switch (variant.type)
		{
		case PrimitiveType::I32:
			_itoa_s(variant.primitive.int32Value, buffer, capacity, radix);
			break;
		case PrimitiveType::I64:
			_i64toa_s(variant.primitive.int64Value, buffer, capacity, radix);
			break;
		case PrimitiveType::U64:
			_ui64toa_s(variant.primitive.int64Value, buffer, capacity, radix);
			break;
		case PrimitiveType::F32:
			snprintf(buffer, capacity, "%f", variant.primitive.float32Value);
			break;
		case PrimitiveType::F64:
			snprintf(buffer, capacity, "%lf", variant.primitive.float64Value);
			break;
		case PrimitiveType::BOOL:
			snprintf(buffer, capacity, "%s", variant.primitive.boolValue ? "true" : "false");
			break;
		case PrimitiveType::CSTR:
			snprintf(buffer, capacity, "%s", variant.stringValue.text.c_str());
			break;
		case PrimitiveType::SUB_OBJECT:
			snprintf(buffer, capacity, "SUB_OBJECT");
			break;
		default:
			snprintf(buffer, capacity, "UNKNOWN-TYPE");
			break;
		}
	}

	struct PreviewData
	{
		PreviewData(PreviewData* _parent, const char* _instanceName = nullptr) : parent(_parent)
		{
			if (_instanceName != nullptr)
			{
				this->instanceName = _instanceName;
			}
		}

		~PreviewData()
		{
			for (auto& i : fields)
			{
				if (i.value.type == PrimitiveType::SUB_OBJECT)
				{
					auto* subObject = i.value.primitive.pSubObject;
					delete subObject;
				}
			}
		}

		PreviewData* parent;
		HString instanceName;
		HString containerKey;
		std::vector<PreviewField> fields;

		template<class T>
		PreviewField& AddField(cstr name, T value)
		{
			fields.push_back(PreviewField());
			auto& back = fields.back();
			back.fieldName = name;
			Assign(back.value, value);
			return back;
		}

		PreviewField& AddField(cstr name, cstr value, int32 capacity)
		{
			fields.push_back(PreviewField());
			auto& back = fields.back();
			back.fieldName = name;
			Assign(back.value, value, capacity);
			return back;
		}
	};

	struct ReflectionEnumerator : IReflectionVisitor
	{
		int fieldCount = 0;
		int subTargetCount = 0;
		int sectionCount = 0;

		void Reflect(cstr name, int32& value, ReflectionMetaData& ) override
		{
			UNUSED(name);
			UNUSED(value);
			fieldCount++;
		}

		void Reflect(cstr name, int64& value, ReflectionMetaData&) override
		{
			UNUSED(name);
			UNUSED(value);
			fieldCount++;
		}

		void Reflect(cstr name, uint64& value, ReflectionMetaData&) override
		{
			UNUSED(name);
			UNUSED(value);
			fieldCount++;
		}

		void Reflect(cstr name, float& value, ReflectionMetaData&) override
		{
			UNUSED(name);
			UNUSED(value);
			fieldCount++;
		}

		void Reflect(cstr name, double& value, ReflectionMetaData&) override
		{
			UNUSED(name);
			UNUSED(value);
			fieldCount++;
		}

		void Reflect(cstr name, bool& value, ReflectionMetaData&) override
		{
			UNUSED(name);
			UNUSED(value);
			fieldCount++;
		}

		void Reflect(cstr name, IReflectedString& stringValue, ReflectionMetaData&) override
		{
			UNUSED(name);
			UNUSED(stringValue);
			fieldCount++;
		}

		void Reflect(cstr name, IReflectionTarget& subTarget, ReflectionMetaData&) override
		{
			UNUSED(name);
			UNUSED(subTarget);
			subTargetCount++;
		}

		void EnterSection(cstr sectionName)
		{
			UNUSED(sectionName);
			sectionCount++;
		}

		void LeaveSection()
		{

		}
	};

	struct Previewer : IReflectionVisitor
	{
		PreviewData* root = nullptr;
		PreviewData* target = nullptr;

		Previewer()
		{
			root = new PreviewData(nullptr);
			target = root;
		}

		~Previewer()
		{
			delete root;
		}

		EReflectionDirection Direction() const override
		{
			return EReflectionDirection::READ_ONLY;
		}

		void Reflect(cstr name, int32& value, ReflectionMetaData&) override
		{
			target->AddField(name, value);
		}

		void Reflect(cstr name, int64& value, ReflectionMetaData&) override
		{
			target->AddField(name, value);
		}

		void Reflect(cstr name, uint64& value, ReflectionMetaData&) override
		{
			target->AddField(name, value);
		}

		void Reflect(cstr name, float& value, ReflectionMetaData&) override
		{
			target->AddField(name, value);
		}

		void Reflect(cstr name, double& value, ReflectionMetaData&) override
		{
			target->AddField(name, value);
		}

		void Reflect(cstr name, bool& value, ReflectionMetaData&) override
		{
			target->AddField(name, value);
		}

		void Reflect(cstr name, IReflectedString& stringValue, ReflectionMetaData&) override
		{
			target->AddField(name, stringValue.ReadString(), stringValue.Capacity());
		}

		void Reflect(cstr name, IReflectionTarget& subTarget, ReflectionMetaData&) override
		{
			auto* subSection = new PreviewData(target);
			target->AddField(name, subSection);
			target = subSection;
			subTarget.Visit(*this);
			target = subSection->parent;
		}

		void EnterSection(cstr sectionName)
		{
			auto* child = new PreviewData(target, sectionName);
			target->AddField(sectionName, child);
			target = child;
		}

		void LeaveSection()
		{
			target = target->parent;
		}

		void EnterContainer(cstr name) override
		{
			auto* subSection = new PreviewData(target);
			target->AddField(name, subSection);
			subSection->parent = target;
			target = subSection;
			target->instanceName = name;
		}

		void LeaveContainer() override
		{
			target = target->parent;
		}

		void EnterElement(cstr keyName) override
		{
			auto* subSection = new PreviewData(target);
			target->AddField(keyName, subSection);
			subSection->parent = target;
			target = subSection;
			target->containerKey = keyName;
		}

		void LeaveElement() override
		{
			target = target->parent;
		}
	};

	struct GRPropertyEditorTree: IGRWidgetPropertyEditorTree, IGRWidgetSupervisor, IGRWidgetCollapserEvents
	{
		IGRPanel& panel;
		Previewer previewer;
		IGRPropertyEditorPopulationEvents& populationEventHandler;

		GRPropertyEditorTree(IGRPanel& owningPanel, IGRPropertyEditorPopulationEvents& _populationEventHandler) : panel(owningPanel), populationEventHandler(_populationEventHandler)
		{
			owningPanel.SetClipChildren(true);
		}

		void Free() override
		{
			delete this;
		}

		void Layout(const GuiRect& panelDimensions) override
		{
			SetCollapserSizes();
			LayoutChildrenByAnchors(panel, panelDimensions);
		}

		void OnCollapserExpanded(IGRWidgetCollapser&) override
		{
			panel.InvalidateLayout(false);
			InvalidateLayoutForAllDescendants(panel);
			SetCollapserSizes();
		}

		void OnCollapserInlined(IGRWidgetCollapser&) override
		{
			panel.InvalidateLayout(false);
			InvalidateLayoutForAllDescendants(panel);
			SetCollapserSizes();
		}

		EGREventRouting OnCursorClick(GRCursorEvent& ce) override
		{
			return viewport->VScroller().Scroller().Widget().OnCursorClick(ce);
		}

		EGREventRouting OnCursorMove(GRCursorEvent& ce) override
		{
			UNUSED(ce);
			return EGREventRouting::NextHandler;
		}

		void OnCursorEnter() override
		{

		}

		void OnCursorLeave() override
		{

		}

		IGRPanel& Panel() override
		{
			return panel;
		}

		void Render(IGRRenderContext& g) override
		{
			auto rect = panel.AbsRect();
			bool isHovered = g.IsHovered(panel);
			UNUSED(rect);
			UNUSED(isHovered);
		}

		EGREventRouting OnChildEvent(GRWidgetEvent&, IGRWidget&)
		{
			return EGREventRouting::NextHandler;
		}

		EGREventRouting OnKeyEvent(GRKeyEvent&) override
		{
			return EGREventRouting::NextHandler;
		}

		EGRQueryInterfaceResult QueryInterface(IGRBase** ppOutputArg, cstr interfaceId) override
		{
			return QueryForParticularInterface<IGRWidgetPropertyEditorTree>(this, ppOutputArg, interfaceId);
		}

		IGRWidget& Widget() override
		{
			return *this;
		}

		struct NameValueControls
		{
			IGRWidgetText& name;
			IGRWidgetEditBox& editor;
		};

		NameValueControls AddFieldToTable(IGRWidgetTable& table, PreviewField& field, int depth)
		{
			UNUSED(depth);

			int newRowIndex = table.AddRow({ 30 });
			auto* nameCell = table.GetCell(0, newRowIndex);

			GRAlignmentFlags nameAlignment;
			nameAlignment.Add(EGRAlignment::VCentre).Add(EGRAlignment::Left);
			auto& nameText = CreateText(*nameCell).SetText(field.fieldName.c_str()).SetAlignment(nameAlignment, { 2,2 });
			nameText.Widget().Panel().Add(GRAnchors::ExpandAll()).Set(GRAnchorPadding{ 4, 0, 0, 0 });

			IGREditFilter* filter = nullptr;

			int32 capacity;

			switch (field.value.type)
			{
			case PrimitiveType::I32:
				filter = &GetI32Filter();
				capacity = 12;
				break;
			case PrimitiveType::I64:
				filter = &GetI64Filter();
				capacity = 24;
				break;
			case PrimitiveType::F32:
				filter = &GetF32Filter();
				capacity = 12;
				break;
			case PrimitiveType::F64:
				filter = &GetF64Filter();
				capacity = 24;
				break;
			case PrimitiveType::CSTR:
				if (field.value.stringValue.capacity > 0x7FFF'FFFFUL)
				{
					table.Widget().Panel().Root().Custodian().RaiseError(table.Widget().Panel().GetAssociatedSExpression(), EGRErrorCode::InvalidArg, __FUNCTION__, "[capacity] > max int32 value");
				}
				capacity = (int32) field.value.stringValue.capacity;
				break;
			default:
				capacity = 10;
				break;
			}

			auto* valueCell = table.GetCell(1, newRowIndex);

			GRAlignmentFlags valueAlignment;
			valueAlignment.Add(EGRAlignment::VCentre).Add(EGRAlignment::Left);
			auto& valueText = CreateEditBox(*valueCell, filter, capacity).SetAlignment(valueAlignment, { 2,2 });
			valueText.Widget().Panel().Add(GRAnchors::ExpandAll()).Set(GRAnchorPadding{ 0, 0, 0, 0 });

			if (field.value.type != PrimitiveType::CSTR)
			{
				char buf[16];
				ToAscii(field.value, buf, sizeof buf);
				valueText.SetText(buf);
			}
			else
			{
				valueText.SetText(field.value.stringValue.text.c_str());
			}

			NameValueControls controls{ nameText, valueText };
			return controls;
		}

		// firstValidIndex and lastValidIndex are required to be valid. Iteration includes the final index
		void AddFieldTable(PreviewData& data, int32 firstValidIndex, int32 lastValidIndex, IGRWidget& parent, int depth)
		{
			auto& table = CreateTable(parent);
			table.Widget().Panel().Set(GRAnchors::ExpandAll());

			GRColumnSpec nameSpec;
			nameSpec.name = "Name";
			nameSpec.maxWidth = 240;
			nameSpec.minWidth = 64;
			nameSpec.defaultWidth = 120;
			table.AddColumn(nameSpec);

			GRColumnSpec valueSpec;
			valueSpec.name = "Value";
			valueSpec.maxWidth = 8192;
			valueSpec.minWidth = 64;
			valueSpec.defaultWidth = 120;
			table.AddColumn(valueSpec);

			table.Widget().Panel().Add(GRAnchors::ExpandAll());

			table.Widget().Panel().Set(EGRSchemeColourSurface::CONTAINER_BACKGROUND, RGBAb(48, 0, 0, 255), GRRenderState(0, 0, 0));

			int nameColumnWidth = nameSpec.defaultWidth;

			for (int32 j = firstValidIndex; j <= lastValidIndex; j++)
			{
				NameValueControls controls = AddFieldToTable(table, data.fields[j], depth);

				int nameWidth = controls.name.GetTextWidth();
				const int padding = 16;
				nameColumnWidth = max(nameWidth + padding, nameColumnWidth);

				populationEventHandler.OnAddNameValue(controls.name, controls.editor);
			}

			table.SetColumnWidth(0, nameColumnWidth);
		}

		void AddSubObject(PreviewField& subObjectField, IGRWidget& parent, int depth)
		{
			SyncUIToPreviewerRecursive(*subObjectField.value.primitive.pSubObject, parent, depth);
		}

		void SyncUIToPreviewerRecursive(PreviewData& data, IGRWidget& parentContainer, int32 depth)
		{
			if (data.fields.empty())
			{
				char message[1024];
				Strings::StackStringBuilder sb(message, sizeof message);
				sb.AppendFormat("[PreviewData& data] had no fields.\n");

				PreviewData* pData = &data;
				while(pData)
				{
					sb.AppendFormat(" under node %s\n", pData->instanceName.length() > 0 ? pData->instanceName.c_str() : "<anon>");
					pData = pData->parent;
				}

				parentContainer.Panel().Root().Custodian().RaiseError(parentContainer.Panel().GetAssociatedSExpression(), EGRErrorCode::BadSpanHeight, __FUNCTION__, "%s", message);
				return;
			}

			auto& collapser = CreateCollapser(parentContainer, *this);
			collapser.Widget().Panel().Set(GRAnchors::ExpandAll());
			collapser.Widget().Panel().Set(GRAnchorPadding{ 8 * depth, 0, 0 , 0 });
			auto& titleDiv = collapser.TitleBar();

			char title[128];
			if (data.containerKey.length() > 0)
			{
				// We must look to the parent, i.e the container, to get the container name
				cstr container = data.parent->instanceName;
				SafeFormat(title, "%s[%s]", container, data.containerKey.c_str());

				collapser.SetExpandClientAreaImagePath("$(COLLAPSER_ELEMENT_EXPAND)");
				collapser.SetCollapsedToInlineImagePath("$(COLLAPSER_ELEMENT_INLINE)");
			}
			else
			{
				SafeFormat(title, "%s", data.instanceName.c_str());
			}

			if (*title == 0)
			{
				SafeFormat(title, "root");
			}

			auto& titleDescription = Rococo::Gui::CreateText(titleDiv).SetText(title);
			titleDescription.Widget().Panel().Add(GRAnchors::ExpandHorizontally()).Add(GRAnchors::ExpandVertically()).Add(GRAnchors::LeftAndRight()).Add(GRAnchors::TopAndBottom()).Set(GRAnchorPadding{ 32, 0, 0, 0 });

			GRAlignmentFlags rightCentered;
			rightCentered.Add(EGRAlignment::Left).Add(EGRAlignment::VCentre);

			titleDescription.SetAlignment(rightCentered, { 0,0 });

			auto& list = CreateVerticalList(collapser.ClientArea());
			list.Widget().Panel().Set(GRAnchors::ExpandAll());

			int32 firstSimpleFieldIndex = -1;
			int32 nextSimpleFieldIndex = -1;

			for (int32 i = 0; i < (int32)data.fields.size(); ++i)
			{
				auto& f = data.fields[i];

				if (f.value.type != PrimitiveType::SUB_OBJECT)
				{
					if (firstSimpleFieldIndex == -1)
					{
						firstSimpleFieldIndex = i;
					}

					if (nextSimpleFieldIndex < i)
					{
						nextSimpleFieldIndex = i;
					}
				}
				else
				{
					if (firstSimpleFieldIndex >= 0)
					{
						AddFieldTable(data, firstSimpleFieldIndex, nextSimpleFieldIndex, list.Widget(), depth);
						firstSimpleFieldIndex = -1;
						nextSimpleFieldIndex = -1;
					}

					AddSubObject(data.fields[i], list.Widget(), depth + 1);
				}
			}

			if (firstSimpleFieldIndex >= 0)
			{
				AddFieldTable(data, firstSimpleFieldIndex, (int32)data.fields.size() - 1, list.Widget(), depth);
			}
		}

		IGRWidgetViewport* viewport = nullptr;

		// Entry point for previewing targets, called by the factory function at the bottom of the page
		void Preview(IReflectionTarget& target)
		{
			target.Visit(previewer);

			viewport = &CreateViewportWidget(*this);

			viewport->SetLineDeltaPixels(30);

			auto& vp = viewport->ClientArea().Panel();
			SetUniformColourForAllRenderStates(vp, EGRSchemeColourSurface::BUTTON_IMAGE_FOG, RGBAb(0, 0, 0, 0));
			vp.Set(EGRSchemeColourSurface::BUTTON_IMAGE_FOG, RGBAb(192, 192, 192, 32), GRRenderState(0, 1, 0));
			vp.Set(EGRSchemeColourSurface::BUTTON_IMAGE_FOG, RGBAb(192, 192, 192, 48), GRRenderState(0, 0, 1));
			vp.Set(EGRSchemeColourSurface::BUTTON_IMAGE_FOG, RGBAb(192, 192, 192, 64), GRRenderState(0, 1, 1));

			GRAnchors anchors = anchors.ExpandAll();

			viewport->Widget().Panel().Set(anchors);

			auto* node = previewer.root;

			if (node) SyncUIToPreviewerRecursive(*node, viewport->ClientArea(), 0);

			SetCollapserSizes();
		}

		int ComputeAndAssignCollapserHeights(IGRWidgetCollapser& collapserParent)
		{
			int32 heightOfDescendants = 30;

			auto* collapserChild = collapserParent.ClientArea().Panel().GetChild(0);

			if (!collapserChild)
			{
				return 0;
			}

			IGRWidgetVerticalList* list = Cast<IGRWidgetVerticalList>(collapserChild->Widget());
			if (!list)
			{
				return 0;
			}
			
			if (!collapserParent.IsCollapsed())
			{
				for (int i = 0; auto * child = list->Widget().Panel().GetChild(i); i++)
				{
					IGRWidgetCollapser* collapser = Cast<IGRWidgetCollapser>(child->Widget());
					if (collapser)
					{
						heightOfDescendants += ComputeAndAssignCollapserHeights(*collapser);
					}
					else
					{
						IGRWidgetTable* table = Cast<IGRWidgetTable>(child->Widget());
						if (table)
						{
							heightOfDescendants += table->Widget().Panel().Span().y;
						}
					}
				}
			}

			collapserParent.Widget().Panel().Resize({ viewport->ClientArea().Panel().Span().x, heightOfDescendants });

			return heightOfDescendants;
		}

		void SetCollapserSizes()
		{
			if (!viewport)
			{
				// Preview was not called, so nothing in the editor to see
				return;
			}
			auto& clientArea = viewport->ClientArea();

			auto* rootCollapserPanel = clientArea.Panel().GetChild(0);
			if (!rootCollapserPanel) return;

			IGRWidgetCollapser* collapser = Cast<IGRWidgetCollapser>(rootCollapserPanel->Widget());
			if (collapser)
			{
				int sumTotalHeight = ComputeAndAssignCollapserHeights(*collapser);
				viewport->SetDomainHeight(sumTotalHeight);
			}
		}

		cstr GetImplementationTypeName() const override
		{
			return "GRPropertyEditorTree";
		}
	};
}

namespace Rococo::Gui
{
	ROCOCO_GUI_RETAINED_API cstr IGRWidgetPropertyEditorTree::InterfaceId()
	{
		return "IGRWidgetPropertyEditorTree";
	}

	ROCOCO_GUI_RETAINED_API IGRWidgetPropertyEditorTree& CreatePropertyEditorTree(IGRWidget& parent, IReflectionTarget& target, IGRPropertyEditorPopulationEvents& populationEventHandler)
	{
		auto& gr = parent.Panel().Root().GR();

		struct GRPropertyEditorTreeFactory : IGRWidgetFactory
		{
			IGRPropertyEditorPopulationEvents& populationEventHandler;

			GRPropertyEditorTreeFactory(IGRPropertyEditorPopulationEvents& _populationEventHandler):
				populationEventHandler(_populationEventHandler)

			{

			}

			IGRWidget& CreateWidget(IGRPanel& panel)
			{
				return *new GRANON::GRPropertyEditorTree(panel, populationEventHandler);
			}
		};

		GRPropertyEditorTreeFactory factory(populationEventHandler);
		auto* tree = static_cast<GRANON::GRPropertyEditorTree*>(Cast<IGRWidgetPropertyEditorTree>(gr.AddWidget(parent.Panel(), factory)));
		tree->Preview(target);
		return *tree;
	}
}