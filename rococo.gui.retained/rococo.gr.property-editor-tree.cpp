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
		I32, I64, F32, F64, BOOL, CSTR, SUB_OBJECT
	};

	union PreviewPrimitive
	{
		int32 int32Value;
		int64 int64Value;
		float float32Value;
		double float64Value;
		bool boolValue;
		PreviewData* pSubObject;
	};

	struct PrimitiveVariant
	{
		HString stringValue;
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

	void Assign(PrimitiveVariant& v, cstr value)
	{
		v.primitive.float64Value = 0;
		v.stringValue = value;
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
			snprintf(buffer, capacity, "%s", variant.stringValue.c_str());
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
		PreviewData(PreviewData* _parent) : parent(_parent)
		{

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
	};

	struct ReflectionEnumerator : IReflectionVisitor
	{
		int fieldCount = 0;
		int subTargetCount = 0;
		int sectionCount = 0;

		void Reflect(cstr name, int32& value, ReflectionMetaData& metaData) override
		{
			fieldCount++;
		}

		void Reflect(cstr name, int64& value, ReflectionMetaData& metaData) override
		{
			fieldCount++;
		}

		void Reflect(cstr name, float& value, ReflectionMetaData& metaData) override
		{
			fieldCount++;
		}

		void Reflect(cstr name, double& value, ReflectionMetaData& metaData) override
		{
			fieldCount++;
		}

		void Reflect(cstr name, bool& value, ReflectionMetaData& metaData) override
		{
			fieldCount++;
		}

		void Reflect(cstr name, IReflectedString& stringValue, ReflectionMetaData& metaData) override
		{
			fieldCount++;
		}

		void Reflect(cstr name, IReflectionTarget& subTarget, ReflectionMetaData& metaData) override
		{
			subTargetCount++;
		}

		void SetSection(cstr sectionName)
		{
			sectionCount++;
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

		void Reflect(cstr name, int32& value, ReflectionMetaData& metaData) override
		{
			target->AddField(name, value);
		}

		void Reflect(cstr name, int64& value, ReflectionMetaData& metaData) override
		{
			target->AddField(name, value);
		}

		void Reflect(cstr name, float& value, ReflectionMetaData& metaData) override
		{
			target->AddField(name, value);
		}

		void Reflect(cstr name, double& value, ReflectionMetaData& metaData) override
		{
			target->AddField(name, value);
		}

		void Reflect(cstr name, bool& value, ReflectionMetaData& metaData) override
		{
			target->AddField(name, value);
		}

		void Reflect(cstr name, IReflectedString& stringValue, ReflectionMetaData& metaData) override
		{
			target->AddField(name, stringValue.ReadString());
		}

		void Reflect(cstr name, IReflectionTarget& subTarget, ReflectionMetaData& metaData) override
		{
			auto* subSection = new PreviewData(target);
			target->AddField(name, subSection);
			target = subSection;
			subTarget.Visit(*this);
			target = subSection->parent;
		}

		void SetSection(cstr sectionName)
		{
			target->instanceName = sectionName;
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

	struct GRPropertyEditorTree: IGRWidgetPropertyEditorTree, IGRWidget
	{
		IGRPanel& panel;

		Previewer previewer;

		GRPropertyEditorTree(IGRPanel& owningPanel) : panel(owningPanel)
		{
		}

		void Free() override
		{
			delete this;
		}

		void Layout(const GuiRect& panelDimensions) override
		{
			LayoutChildrenByAnchors(panel, panelDimensions);
		//	SetCollapserSizes();
		}

		EventRouting OnCursorClick(CursorEvent& ce) override
		{
			return EventRouting::NextHandler;
		}

		EventRouting OnCursorMove(CursorEvent& ce) override
		{
			return EventRouting::NextHandler;
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
		}

		EventRouting OnChildEvent(WidgetEvent& widgetEvent, IGRWidget& sourceWidget)
		{
			return EventRouting::NextHandler;
		}

		EventRouting OnKeyEvent(KeyEvent& keyEvent) override
		{
			return EventRouting::NextHandler;
		}

		Vec2i EvaluateMinimalSpan() const override
		{
			return { 0,0 };
		}

		EQueryInterfaceResult QueryInterface(IGRBase** ppOutputArg, cstr interfaceId) override
		{
			return QueryForParticularInterface<IGRWidgetPropertyEditorTree>(this, ppOutputArg, interfaceId);
		}

		IGRWidget& Widget() override
		{
			return *this;
		}

		void AddFieldToTable(IGRWidgetTable& table, PreviewField& field, int depth)
		{
			int newRowIndex = table.AddRow({ 30 });
			auto* nameCell = table.GetCell(0, newRowIndex);

			GRAlignmentFlags nameAlignment;
			nameAlignment.Add(GRAlignment::VCentre).Add(GRAlignment::Left);
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
			default:
				capacity = 10;
				break;
			}

			auto* valueCell = table.GetCell(1, newRowIndex);

			GRAlignmentFlags valueAlignment;
			valueAlignment.Add(GRAlignment::VCentre).Add(GRAlignment::Left);
			auto& valueText = CreateEditBox(*valueCell, filter, capacity).SetAlignment(valueAlignment, { 2,2 });
			valueText.Widget().Panel().Add(GRAnchors::ExpandAll()).Set(GRAnchorPadding{ 0, 0, 0, 0 });

			char buf[16];
			ToAscii(field.value, buf, sizeof buf);
			valueText.SetText(buf);
		}

		// firstValidIndex and lastValidIndex are required to be valid. Iteration includes the final index
		void AddFieldTable(PreviewData& data, int32 firstValidIndex, int32 lastValidIndex, IGRWidget& parent, int depth, int& accumulatedHeight)
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

			table.Widget().Panel().Set(ESchemeColourSurface::CONTAINER_BACKGROUND, RGBAb(48, 0, 0, 255));
			table.Widget().Panel().Set(ESchemeColourSurface::CONTAINER_BACKGROUND_HOVERED, RGBAb(50, 0, 0, 255));

			for (int32 j = firstValidIndex; j <= lastValidIndex; j++)
			{
				AddFieldToTable(table, data.fields[j], depth);
			}

			accumulatedHeight += table.Widget().Panel().Span().y;
		}

		void AddSubObject(PreviewField& subObjectField, IGRWidget& parent, int depth, int& accumulatedHeight)
		{
			SyncUIToPreviewerRecursive(*subObjectField.value.primitive.pSubObject, parent, depth, accumulatedHeight);
		}

		void SyncUIToPreviewerRecursive(PreviewData& data, IGRWidget& parentContainer, int32 depth, int& accumulatedParentHeight)
		{
			auto& collapser = CreateCollapser(parentContainer);
			collapser.Widget().Panel().Set(GRAnchors::ExpandAll());
			collapser.Widget().Panel().Set(GRAnchorPadding{ 8 * depth, 0, 0 , 0 });
			auto& titleDiv = collapser.TitleBar();

			char title[128];
			if (data.containerKey.length() > 0)
			{
				// We must look to the parent, i.e the container, to get the container name
				cstr container = data.parent->instanceName;
				SafeFormat(title, "%s[%s]", container, data.containerKey.c_str());
			}
			else
			{
				SafeFormat(title, "%s", data.instanceName.c_str());
			}

			auto& titleDescription = Rococo::Gui::CreateText(titleDiv).SetText(title);
			titleDescription.Widget().Panel().Add(GRAnchors::ExpandHorizontally()).Add(GRAnchors::ExpandVertically()).Add(GRAnchors::LeftAndRight()).Add(GRAnchors::TopAndBottom()).Set(GRAnchorPadding{ 32, 0, 0, 0 });

			GRAlignmentFlags rightCentered;
			rightCentered.Add(GRAlignment::Left).Add(GRAlignment::VCentre);

			titleDescription.SetAlignment(rightCentered, { 0,0 });

			auto& list = CreateVerticalList(collapser.ClientArea());
			list.Panel().Set(GRAnchors::ExpandAll());

			int32 firstSimpleFieldIndex = -1;
			int32 nextSimpleFieldIndex = -1;

			int32 accumulatedCollapserAreaHeight = 30;

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
						AddFieldTable(data, firstSimpleFieldIndex, nextSimpleFieldIndex, list, depth, accumulatedCollapserAreaHeight);
						firstSimpleFieldIndex = -1;
						nextSimpleFieldIndex = -1;
					}

					AddSubObject(data.fields[i], list, depth + 1, accumulatedCollapserAreaHeight);
				}
			}

			if (firstSimpleFieldIndex >= 0)
			{
				AddFieldTable(data, firstSimpleFieldIndex, (int32)data.fields.size() - 1, list, depth, accumulatedCollapserAreaHeight);
			}

			collapser.Widget().Panel().Resize({ 0, accumulatedCollapserAreaHeight });

			accumulatedParentHeight += accumulatedCollapserAreaHeight;
		}

		IGRWidgetViewport* viewport = nullptr;

		void Preview(IReflectionTarget& target)
		{
			target.Visit(previewer);

			viewport = &CreateViewportWidget(*this);

			GRAnchors anchors = anchors.ExpandAll();

			viewport->Widget().Panel().Set(anchors);

			auto* node = previewer.root;

			int32 accumulatedHeight = 0;
			if (node) SyncUIToPreviewerRecursive(*node, viewport->ClientArea(), 0, accumulatedHeight);

			SetCollapserSizes();
		}

		int ComputeAndAssignCollapserHeights(IGRWidgetCollapser& collapserParent)
		{
			int32 heightOfDescendants = 0;

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

			for(int i = 0; auto* child = list->Panel().GetChild(i); i++)
			{
				IGRWidgetCollapser* collapser = Cast<IGRWidgetCollapser>(child->Widget());
				if (collapser)
				{
					heightOfDescendants += 30 + ComputeAndAssignCollapserHeights(*collapser);
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

			collapserParent.Widget().Panel().Resize({ viewport->ClientArea().Panel().Span().x, heightOfDescendants });

			return heightOfDescendants;
		}

		void SetCollapserSizes()
		{
			auto& clientArea = viewport->ClientArea();

			auto* rootCollapserPanel = clientArea.Panel().GetChild(0);
			if (!rootCollapserPanel) return;

			IGRWidgetCollapser* collapser = Cast<IGRWidgetCollapser>(rootCollapserPanel->Widget());
			if (collapser)
			{
				int sumTotalHeight = ComputeAndAssignCollapserHeights(*collapser);
			}
		}
	};
}

namespace Rococo::Gui
{
	ROCOCO_GUI_RETAINED_API cstr IGRWidgetPropertyEditorTree::InterfaceId()
	{
		return "IGRWidgetPropertyEditorTree";
	}

	ROCOCO_GUI_RETAINED_API IGRWidgetPropertyEditorTree& CreatePropertyEditorTree(IGRWidget& parent, IReflectionTarget& target)
	{
		auto& gr = parent.Panel().Root().GR();

		struct GRPropertyEditorTreeFactory : IGRWidgetFactory
		{
			IGRWidget& CreateWidget(IGRPanel& panel)
			{
				return *new GRANON::GRPropertyEditorTree(panel);
			}
		};
		
		static GRPropertyEditorTreeFactory editorFactory;

		auto* tree = static_cast<GRANON::GRPropertyEditorTree*>(Cast<IGRWidgetPropertyEditorTree>(gr.AddWidget(parent.Panel(), editorFactory)));
		tree->Preview(target);
		return *tree;
	}
}