#include <rococo.gui.retained.ex.h>
#include <rococo.maths.i32.h>
#include <string>
#include <rococo.reflector.h>
#include <rococo.strings.h>
#include <rococo.formatting.h>
#include <vector>
#include <unordered_map>
#include <rococo.ui.h>
#include <rococo.vkeys.h>

using namespace Rococo;
using namespace Rococo::Gui;
using namespace Rococo::Reflection;
using namespace Rococo::Strings;

namespace GRANON
{
	struct PreviewData;

	enum class PrimitiveType
	{
		I32, I64, U64, F32, F64, BOOL, CSTR, HSTR, SUB_OBJECT
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

		PreviewPrimitive(): uint64Value(0)
		{

		}
	};

	struct EditableString
	{
		HString text;
		size_t capacity = 0;
	};

	struct PrimitiveVariant
	{
		EditableString stringValue;
		PreviewPrimitive primitive;
		PrimitiveType type = PrimitiveType::BOOL;

		// Pointer to the primitive from which this was copied. If the pointer is volatile or temporary, then the orgin will remain null
		void* primitiveOrigin = nullptr;
	};

	void Assign(PrimitiveVariant& v, int32& value, bool retainOrigin)
	{
		v.primitive.int32Value = value;
		v.type = PrimitiveType::I32;
		if (retainOrigin)
		{
			v.primitiveOrigin = &value;
		}
	}

	void Assign(PrimitiveVariant& v, int64& value, bool retainOrigin)
	{
		v.primitive.int64Value = value;
		v.type = PrimitiveType::I64;
		if (retainOrigin)
		{
			v.primitiveOrigin = &value;
		}
	}

	void Assign(PrimitiveVariant& v, uint64& value, bool retainOrigin)
	{
		v.primitive.uint64Value = value;
		v.type = PrimitiveType::U64;
		if (retainOrigin)
		{
			v.primitiveOrigin = &value;
		}
	}

	void Assign(PrimitiveVariant& v, float& value, bool retainOrigin)
	{
		v.primitive.float32Value = value;
		v.type = PrimitiveType::F32;
		if (retainOrigin)
		{
			v.primitiveOrigin = &value;
		}
	}

	void Assign(PrimitiveVariant& v, double& value, bool retainOrigin)
	{
		v.primitive.float64Value = value;
		v.type = PrimitiveType::F64;
		if (retainOrigin)
		{
			v.primitiveOrigin = &value;
		}
	}

	void Assign(PrimitiveVariant& v, bool& value, bool retainOrigin)
	{
		v.primitive.boolValue = value;
		v.type = PrimitiveType::BOOL;
		if (retainOrigin)
		{
			v.primitiveOrigin = &value;
		}
	}

	void Assign(PrimitiveVariant& v, char* value, size_t capacity, bool retainOrigin)
	{
		v.primitive.float64Value = 0;
		v.stringValue.text = value;
		v.stringValue.capacity = capacity;
		v.type = PrimitiveType::CSTR;
		if (retainOrigin)
		{
			v.primitiveOrigin = value;
		}
	}

	void Assign(PrimitiveVariant& v, HString& stringRef, bool retainOrigin)
	{
		v.primitive.float64Value = 0;
		v.stringValue.text = stringRef.c_str();
		v.stringValue.capacity = Rococo::max(4096ULL, stringRef.length() + 1ULL);
		v.type = PrimitiveType::HSTR;
		if (retainOrigin)
		{
			v.primitiveOrigin = &stringRef;
		}
	}

	void Assign(PrimitiveVariant& v, PreviewData* subObject, bool retainOrigin)
	{
		v.primitive.pSubObject = subObject;
		v.type = PrimitiveType::SUB_OBJECT;
		if (retainOrigin)
		{
			v.primitiveOrigin = subObject;
		}		
	}

	enum EParseAndWriteBackResult
	{
		Success,
		NoOrigin,
		UnknownPrimitiveType
	};

	void RemoveRedundantZeros(char* buffer)
	{
		size_t len = strlen(buffer);

		char* end = buffer + len - 1;

		const char* decimalPos = FindChar(buffer, '.');
		if (decimalPos)
		{
			for (char* zeros = end; zeros > decimalPos + 1; zeros--)
			{
				if (*zeros == '0')
				{
					*zeros = 0;
				}
				else
				{
					break;
				}
			}
		}
	}

	struct PreviewField
	{
		HString fieldName;
		PrimitiveVariant value;
		ReflectionMetaData meta;

		[[nodiscard]] EParseAndWriteBackResult TryParseAndWriteBackToOrigin(cstr text, IGRWidgetEditBox& sender)
		{
			if (!value.primitiveOrigin)
			{
				return EParseAndWriteBackResult::NoOrigin;
			}

			switch (value.type)
			{
				case PrimitiveType::I32:
				{
					Formatting::TryParseResult<int32> result = Formatting::TryParseInt32FromDecimalStringSkippingCetera(text);
					auto* origin = reinterpret_cast<int*>(value.primitiveOrigin);
					*origin = meta.hasMinmax ? clamp(result.Value, meta.min.i32Value, meta.max.i32Value) : result.Value;

					char buffer[16];
					Formatting::ToAscii(*origin, 10, meta.addThousandMarks, ',', buffer, sizeof(buffer));
					sender.SetText(buffer);
					return EParseAndWriteBackResult::Success;
				}
				case PrimitiveType::I64:
				{
					Formatting::TryParseResult<int64> result = Formatting::TryParseInt64FromDecimalStringSkippingCetera(text);
					auto* origin = reinterpret_cast<int64*>(value.primitiveOrigin);
					*origin = meta.hasMinmax ? clamp(result.Value, meta.min.i64Value, meta.max.i64Value) : result.Value;

					char buffer[32];
					Formatting::ToAscii(*origin, 10, meta.addThousandMarks, ',', buffer, sizeof(buffer));
					sender.SetText(buffer);
					return EParseAndWriteBackResult::Success;
				}
				case PrimitiveType::F32:
				{
					float f = (float) atof(text);
					auto* origin = reinterpret_cast<float*>(value.primitiveOrigin);

					float f1;

					if (isnan(f))
					{
						f1 = meta.min.f32Value;
					}
					else if (isinf(f))
					{
						f1 = meta.max.f32Value;
					}
					else
					{
						f1 = meta.hasMinmax ? clamp(f, meta.min.f32Value, meta.max.f32Value) : f;
					}

					*origin = f1;
					char buffer[32];
					SafeFormat(buffer, "%f", *origin);
					RemoveRedundantZeros(buffer);
					sender.SetText(buffer);
					return EParseAndWriteBackResult::Success;
				}
				case PrimitiveType::F64:
				{
					double d = atof(text);
					auto* origin = reinterpret_cast<double*>(value.primitiveOrigin);
		
					double d1;
					if (isnan(d))
					{
						d1 = meta.min.f32Value;
					}
					else if (isinf(d))
					{
						d1 = meta.max.f32Value;
					}
					else
					{
						d1 = meta.hasMinmax ? clamp(d, meta.min.f64Value, meta.max.f64Value): d;
					}

					*origin = d1;
					char buffer[32];
					SafeFormat(buffer, "%f", *origin);
					RemoveRedundantZeros(buffer);
					sender.SetText(buffer);
					return EParseAndWriteBackResult::Success;
				}
				case PrimitiveType::HSTR:
				{
					auto* origin = reinterpret_cast<HString*>(value.primitiveOrigin);
					*origin = text;
					return EParseAndWriteBackResult::Success;
				}
				case PrimitiveType::CSTR:
				{
					auto* origin = reinterpret_cast<char*>(value.primitiveOrigin);
					CopyString(origin, value.stringValue.capacity, text);
					return EParseAndWriteBackResult::Success;
				}
			default:
				return EParseAndWriteBackResult::UnknownPrimitiveType;
			}
		}
	};

	bool ToAscii(const PrimitiveVariant& variant, char* buffer, size_t capacity, const ReflectionMetaData& meta, int32 radix = 10)
	{
		char format[8] = "%.nAB";

		char precision = (char) clamp(meta.precision, 0, 9);
		format[2] = precision + '0';

		switch (variant.type)
		{
		case PrimitiveType::F32:
			format[3] = 'f';
			format[4] = 0;
			break;
		case PrimitiveType::F64:
			format[3] = 'l';
			format[4] = 'f';
			format[5] = 0;
			break;
		}

		switch (variant.type)
		{
		case PrimitiveType::I32:
			return Formatting::ToAscii(variant.primitive.int32Value, radix, meta.addThousandMarks, ',', buffer, capacity);
		case PrimitiveType::I64:
			return _i64toa_s(variant.primitive.int64Value, buffer, capacity, radix) != 0;
		case PrimitiveType::U64:
			return _ui64toa_s(variant.primitive.int64Value, buffer, capacity, radix) != 0;
		case PrimitiveType::F32:
			return snprintf(buffer, capacity, format, variant.primitive.float32Value) > 0;
		case PrimitiveType::F64:
			return snprintf(buffer, capacity, format, variant.primitive.float64Value) > 0;
		case PrimitiveType::BOOL:
			return snprintf(buffer, capacity, "%s", variant.primitive.boolValue ? "true" : "false") > 0;
		case PrimitiveType::CSTR:
			return snprintf(buffer, capacity, "%s", variant.stringValue.text.c_str()) > 0;
		case PrimitiveType::HSTR:
			return snprintf(buffer, capacity, "%s", variant.stringValue.text.c_str()) > 0;
		case PrimitiveType::SUB_OBJECT:
			return snprintf(buffer, capacity, "SUB_OBJECT") > 0;
		default:
			return snprintf(buffer, capacity, "UNKNOWN-TYPE") > 0;			
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
				if (i->value.type == PrimitiveType::SUB_OBJECT)
				{
					auto* subObject = i->value.primitive.pSubObject;
					delete subObject;
				}

				delete i;
			}
		}

		PreviewData* parent;
		HString instanceName;
		HString containerKey;
		std::vector<PreviewField*> fields;

		template<class T>
		PreviewField& AddField(cstr name, T& value, const Reflection::ReflectionMetaData& meta)
		{
			fields.push_back(new PreviewField());
			auto& back = *fields.back();
			back.fieldName = name;
			back.meta = meta;
			Assign(back.value, value, true);
			return back;
		}

		PreviewField& AddField(cstr name, char* value, size_t capacity, const Reflection::ReflectionMetaData& meta)
		{
			fields.push_back(new PreviewField());
			auto& back = *fields.back();
			back.fieldName = name;
			back.meta = meta;
			Assign(back.value, value, capacity, true);
			return back;
		}

		void CancelVisitRecursive()
		{
			for (auto* f : fields)
			{
				f->value.primitiveOrigin = nullptr;

				if (f->value.type == PrimitiveType::SUB_OBJECT)
				{
					f->value.primitive.pSubObject->CancelVisitRecursive();
				}
			}
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

		void Reflect(cstr name, char* buffer, size_t capacity, ReflectionMetaData&) override
		{
			UNUSED(name);
			UNUSED(buffer);
			UNUSED(capacity);
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

	struct GRPropertyEditorTree;

	struct Previewer : IReflectionVisitor
	{
		GRPropertyEditorTree& owner;
		PreviewData* root = nullptr;
		PreviewData* target = nullptr;

		Previewer(GRPropertyEditorTree& _owner): owner(_owner)
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

		void Reflect(cstr name, int32& value, ReflectionMetaData& meta) override
		{
			target->AddField(name, value, meta);
		}

		void Reflect(cstr name, int64& value, ReflectionMetaData& meta) override
		{
			target->AddField(name, value, meta);
		}

		void Reflect(cstr name, uint64& value, ReflectionMetaData& meta) override
		{
			target->AddField(name, value, meta);
		}

		void Reflect(cstr name, float& value, ReflectionMetaData& meta) override
		{
			target->AddField(name, value, meta);
		}

		void Reflect(cstr name, double& value, ReflectionMetaData& meta) override
		{
			target->AddField(name, value, meta);
		}

		void Reflect(cstr name, bool& value, ReflectionMetaData& meta) override
		{
			target->AddField(name, value, meta);
		}

		void Reflect(cstr name, char* stringBuffer, size_t capacity, ReflectionMetaData& meta) override
		{
			target->AddField(name, stringBuffer, capacity, meta);
		}

		void Reflect(cstr name, Strings::HString& stringRef, ReflectionMetaData& metaData) override
		{
			target->AddField(name, stringRef, metaData);
		}

		void Reflect(cstr name, IReflectionTarget& subTarget, ReflectionMetaData& meta) override
		{
			auto* subSection = new PreviewData(target);
			target->AddField(name, subSection, meta);
			target = subSection;
			subTarget.Visit(*this);
			target = subSection->parent;
		}

		void EnterSection(cstr sectionName)
		{
			auto* child = new PreviewData(target, sectionName);
			target->AddField(sectionName, child, ReflectionMetaData::Default());
			target = child;
		}

		void LeaveSection()
		{
			target = target->parent;
		}

		void EnterContainer(cstr name) override
		{
			auto* subSection = new PreviewData(target);
			target->AddField(name, subSection, ReflectionMetaData::Default());
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
			target->AddField(keyName, subSection, ReflectionMetaData::Default());
			subSection->parent = target;
			target = subSection;
			target->containerKey = keyName;
		}

		void LeaveElement() override
		{
			target = target->parent;
		}

		void CancelVisit(IReflectionVisitation& visitation) override;
	};

	void SetEditorsToReadOnlyRecursive(IGRPanel& panel)
	{
		auto* editor = Cast<IGRWidgetEditBox>(panel.Widget());
		if (editor)
		{
			editor->SetReadOnly(true);
		}

		int nChildren = panel.EnumerateChildren(nullptr);
		for (int i = 0; i < nChildren; i++)
		{
			auto* child = panel.GetChild(i);
			SetEditorsToReadOnlyRecursive(*child);
		}
	}

	struct GRPropertyEditorTree: IGRWidgetPropertyEditorTree, IGRWidgetSupervisor, IGRWidgetCollapserEvents
	{
		IGRPanel& panel;
		Previewer previewer;
		IGRPropertyEditorPopulationEvents& populationEventHandler;
		PropertyEditorSpec spec;

		std::unordered_map<size_t, PreviewField*> editorToPreviewField;

		GRPropertyEditorTree(IGRPanel& panel, IGRPropertyEditorPopulationEvents& _populationEventHandler, const PropertyEditorSpec& _spec) :
			panel(panel), previewer(*this), populationEventHandler(_populationEventHandler), spec(_spec)
		{
			panel.SetClipChildren(true);
			panel.SetExpandToParentHorizontally();
			panel.SetExpandToParentVertically();

			if (spec.NameplateFontId == GRFontId::NONE)
			{
				FontSpec boldFont;
				boldFont.Bold = true;
				boldFont.CharHeight = 14;
				boldFont.CharSet = ECharSet::ANSI;
				boldFont.FontName = "Consolas";
				spec.NameplateFontId = GetCustodian(panel).Fonts().BindFontId(boldFont);
			}

			if (spec.HeadingFontId == GRFontId::NONE)
			{
				FontSpec headingFontSpec;
				headingFontSpec.Bold = true;
				headingFontSpec.CharHeight = 16;
				headingFontSpec.CharSet = ECharSet::ANSI;
				headingFontSpec.FontName = "Consolas";
				spec.HeadingFontId = GetCustodian(panel).Fonts().BindFontId(headingFontSpec);
			}

			if (spec.ValueFontId == GRFontId::NONE)
			{
				FontSpec valueFontSpec;
				valueFontSpec.Bold = false;
				valueFontSpec.CharHeight = 14;
				valueFontSpec.CharSet = ECharSet::ANSI;
				valueFontSpec.FontName = "Consolas";
				spec.ValueFontId = GetCustodian(panel).Fonts().BindFontId(valueFontSpec);
			}
		}

		virtual ~GRPropertyEditorTree()
		{
			if (currentVisitation)
			{
				currentVisitation->OnVisitorGone(previewer);
			}
		}

		void Free() override
		{
			delete this;
		}

		bool CancelVisit(IReflectionVisitation& visitation)
		{
			if (currentVisitation == &visitation)
			{
				currentVisitation = nullptr;
				SetEditorsToReadOnlyRecursive(panel);
				return true;
			}

			return false;
		}

		void OnCollapserExpanded(IGRWidgetCollapser&) override
		{
			SetCollapserSizes();
		}

		void OnCollapserInlined(IGRWidgetCollapser&) override
		{
			SetCollapserSizes();
		}

		EGREventRouting OnCursorClick(GRCursorEvent& ce) override
		{
			return viewport->VScroller().Scroller().Widget().Manager().OnCursorClick(ce);
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

		void HilightFocusedCollapserRecursive(IGRPanel& p)
		{
			auto* collapser = Cast<IGRWidgetCollapser>(p.Widget());
			if (collapser)
			{
				auto* textPanel = collapser->TitleBar().Panel().GetChild(2);
				if (textPanel)
				{
					auto* text = Cast<IGRWidgetText>(textPanel->Widget());
					if (p.HasFocus())
					{
						text->SetBackColourSurface(EGRSchemeColourSurface::FOCUS_RECTANGLE);
					}
					else
					{
						text->SetBackColourSurface(EGRSchemeColourSurface::CONTAINER_BACKGROUND);
					}
				}
			}

			int nChildren = p.EnumerateChildren(nullptr);
			for (int i = 0; i < nChildren; i++)
			{
				auto* child = p.GetChild(i);
				HilightFocusedCollapserRecursive(*child);
			}
		}

		void Render(IGRRenderContext& g) override
		{
			auto rect = panel.AbsRect();
			bool isHovered = g.IsHovered(panel);
			RGBAb colour = panel.GetColour(Gui::EGRSchemeColourSurface::CONTAINER_BACKGROUND, GRWidgetRenderState(false, isHovered, false));
			g.DrawRect(rect, colour);

			HilightFocusedCollapserRecursive(panel);
		}

		void UpdateValueFrom(IGRWidgetEditBox& editor)
		{
			size_t key = reinterpret_cast<size_t>(&editor);
			auto i = editorToPreviewField.find(key);
			if (i == editorToPreviewField.end())
			{
				return;
			}

			char text[256];
			editor.GetTextAndLength(text, sizeof(text));

			auto result = i->second->TryParseAndWriteBackToOrigin(text, editor);
			if (result != EParseAndWriteBackResult::Success)
			{
				RaiseError(editor.Panel(), EGRErrorCode::Generic, __ROCOCO_FUNCTION__, "TryParseAndWriteBackToOrigin failed with code %d", result);
			}
		}

		EGREventRouting OnEditorUpdated(GRWidgetEvent_EditorUpdated& update, IGRWidget& sender)
		{
			UNUSED(sender);
			IGRWidgetEditBox& editor = *update.editor;
			if (update.editorEventType == EGREditorEventType::LostFocus)
			{
				UpdateValueFrom(editor);
			}
			return EGREventRouting::Terminate;
		}

		EGREventRouting OnChildEvent(GRWidgetEvent& ev, IGRWidget& sender)
		{
			if (ev.eventType == EGRWidgetEventType::EDITOR_UPDATED)
			{
				auto& editorEv = static_cast<GRWidgetEvent_EditorUpdated&>(ev);
				return OnEditorUpdated(editorEv, sender);
			}
			return EGREventRouting::NextHandler;
		}

		EGREventRouting MoveToSiblingCollapserIfFocused()
		{
			auto* focusWidget = panel.Root().GR().FindFocusWidget();
			if (!focusWidget)
			{
				return EGREventRouting::NextHandler;
			}

			auto* collapser = Cast<IGRWidgetCollapser>(*focusWidget);
			if (!collapser)
			{
				return EGREventRouting::NextHandler;
			}

			if (collapser->Panel().Parent()->EnumerateChildren(nullptr) == 1)
			{
				// Collapser has no siblings, so we prevent focus moving
				return EGREventRouting::Terminate;
			}

			return EGREventRouting::NextHandler;
		}

		EGREventRouting OnKeyEvent(GRKeyEvent& ke) override
		{
			if (ke.osKeyEvent.IsUp())
			{
				switch (ke.osKeyEvent.VKey)
				{
				case IO::VirtualKeys::VKCode_ANTITAB:
				case IO::VirtualKeys::VKCode_UP:
				case IO::VirtualKeys::VKCode_DOWN:
				case IO::VirtualKeys::VKCode_TAB:
					return EGREventRouting::Terminate;
				}
				return EGREventRouting::NextHandler;
			}			

			// Key down or repeat
			switch (ke.osKeyEvent.VKey)
			{
			case IO::VirtualKeys::VKCode_TAB:
				return MoveToSiblingCollapserIfFocused();
			}

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

		IGRWidgetEditBox* firstCreatedEditBox = nullptr;
		IGRWidgetEditBox* lastCreatedEditBox = nullptr;

		NameValueControls AddFieldToTable(IGRWidgetTable& table, PreviewField& field, int rowHeight, int depth)
		{
			UNUSED(depth);

			int newRowIndex = table.AddRow(GRRowSpec{ rowHeight });
			auto* nameCell = table.GetCell(0, newRowIndex);

			auto rowSurface = (newRowIndex % 2 == 0) ? EGRSchemeColourSurface::ROW_COLOUR_EVEN : EGRSchemeColourSurface::ROW_COLOUR_ODD;

			auto& rowPanel = *nameCell->Panel().Parent();
			CopyAllColours(rowPanel, rowPanel, rowSurface, EGRSchemeColourSurface::CONTAINER_BACKGROUND);
			CopyAllColours(rowPanel, rowPanel, rowSurface, EGRSchemeColourSurface::CONTAINER_TOP_LEFT);
			CopyAllColours(rowPanel, rowPanel, rowSurface, EGRSchemeColourSurface::CONTAINER_BOTTOM_RIGHT);

			GRAlignmentFlags nameAlignment;
			nameAlignment.Add(EGRAlignment::VCentre).Add(spec.LeftAlignNameplates ? EGRAlignment::Left : EGRAlignment::Right);
			auto& leftSpacer = CreateDivision(nameCell->Widget());
			leftSpacer.SetTransparency(0);
			leftSpacer.Panel().SetConstantWidth(spec.LeftHandMargin + spec.LeftHandShiftPerDepth * depth).SetDesc("NameCell-LeftSpacer");

			char label[256];
			Strings::SafeFormat(label, "%s:", field.fieldName.c_str());
			auto& nameText = CreateText(nameCell->Widget()).SetText(label).SetAlignment(nameAlignment, spec.NameTextSpacing);
			nameText.SetFont(spec.NameplateFontId);
			nameText.Panel().SetExpandToParentHorizontally();
			nameText.Panel().SetExpandToParentVertically();
			nameText.Panel().Set(spec.NameCellPadding);

			Strings::SafeFormat(label, "NameCell: %s", field.fieldName.c_str());

			nameCell->Panel().SetDesc(label);;

			auto& namePanel = nameText.Panel();
			CopyAllColours(namePanel, namePanel, EGRSchemeColourSurface::NAME_TEXT, EGRSchemeColourSurface::TEXT);

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
					RaiseError(table.Panel(), EGRErrorCode::InvalidArg, __ROCOCO_FUNCTION__, "[capacity] > max int32 value");
				}
				capacity = (int32)field.value.stringValue.capacity;
				break;
			case PrimitiveType::HSTR:
				capacity = (int32)field.value.stringValue.capacity;
				break;
			default:
				capacity = 1;
				RaiseError(panel, EGRErrorCode::InvalidArg, __ROCOCO_FUNCTION__, "Bad field value type: %d", field.value.type);
			}

			auto* valueCell = table.GetCell(1, newRowIndex);

			auto& valuePanel = valueCell->Panel();
			CopyAllColours(valuePanel, valuePanel, rowSurface, EGRSchemeColourSurface::CONTAINER_BACKGROUND);
			CopyAllColours(valuePanel, valuePanel, rowSurface, EGRSchemeColourSurface::CONTAINER_TOP_LEFT);
			CopyAllColours(valuePanel, valuePanel, rowSurface, EGRSchemeColourSurface::CONTAINER_BOTTOM_RIGHT);
			CopyAllColours(valuePanel, valuePanel, EGRSchemeColourSurface::VALUE_TEXT, EGRSchemeColourSurface::TEXT);

			char description[64];

			GRAlignmentFlags valueAlignment;
			valueAlignment.Add(EGRAlignment::VCentre).Add(EGRAlignment::Left);
			auto& valueText = CreateEditBox(valueCell->Widget(), filter, capacity, spec.ValueFontId).SetAlignment(valueAlignment, spec.EditorCellPadding);

			SafeFormat(description, "editor %llx", valueText.Panel().Id());
			valueText.Panel().SetDesc(description);

			if (lastCreatedEditBox)
			{
				lastCreatedEditBox->Panel().Set(EGRNavigationDirection::Down, description);
				valueText.Panel().Set(EGRNavigationDirection::Up, lastCreatedEditBox->Panel().Desc());
			}

			lastCreatedEditBox = &valueText;
			if (firstCreatedEditBox == nullptr)
			{
				firstCreatedEditBox = lastCreatedEditBox;
			}

			valueText.Panel().Set(spec.ValueCellPadding);
			valueText.Panel().SetExpandToParentHorizontally();
			valueText.Panel().SetExpandToParentVertically();

			if (field.meta.isReadOnly)
			{
				valueText.SetReadOnly(true);
			}

			if (field.value.type != PrimitiveType::CSTR && field.value.type != PrimitiveType::HSTR)
			{
				char buf[16];
				if (!ToAscii(field.value, buf, sizeof(buf), field.meta))
				{	
					RaiseError(panel, EGRErrorCode::InvalidArg, __ROCOCO_FUNCTION__, "Could not get an ascii representation of the field value");
				}
				valueText.SetText(buf);
			}
			else
			{
				valueText.SetText(field.value.stringValue.text.c_str());
			}

			NameValueControls controls{ nameText, valueText };

			size_t key = reinterpret_cast<size_t>(&valueText);
			editorToPreviewField[key] = &field;

			return controls;
		}

		std::vector<int> maxColumnWidthByDepth;

		struct TableList
		{
			std::vector<IGRWidgetTable*> tables;
		};

		std::vector<TableList> tableByDepth;

		// firstValidIndex and lastValidIndex are required to be valid. Iteration includes the final index
		void AddFieldTable(PreviewData& data, int32 firstValidIndex, int32 lastValidIndex, IGRWidget& parent, int depth)
		{
			auto& table = CreateTable(parent);
			table.Panel().SetExpandToParentHorizontally();
			table.Panel().SetExpandToParentVertically();
			table.Panel().SetLayoutDirection(ELayoutDirection::TopToBottom);

			GRColumnSpec nameSpec;
			nameSpec.name = "Name";
			nameSpec.maxWidth = 240;
			nameSpec.minWidth = 64;
			nameSpec.defaultWidth = spec.NameColumnDefaultWidth;
			table.AddColumn(nameSpec);

			GRColumnSpec valueSpec;
			valueSpec.name = "Value";
			valueSpec.maxWidth = 8192;
			valueSpec.minWidth = 64;
			valueSpec.defaultWidth = spec.ValueColumnDefaultWidth;
			table.AddColumn(valueSpec);

			SetUniformColourForAllRenderStates(table.Panel(), EGRSchemeColourSurface::CONTAINER_BACKGROUND, RGBAb(0, 0, 0, 0));

			int nameColumnWidth = nameSpec.defaultWidth;

			for (int32 j = firstValidIndex; j <= lastValidIndex; j++)
			{
				NameValueControls controls = AddFieldToTable(table, *data.fields[j], rowHeight, depth);

				int nameWidth = controls.name.TextWidth();
				auto* spacer = controls.name.Panel().Parent()->GetChild(0);
				const int padding = spacer->Span().x + spec.NamePlateSafeZone;
				nameColumnWidth = Rococo::max(nameWidth + padding, nameColumnWidth);

				populationEventHandler.OnAddNameValue(controls.name, controls.editor);
			}

			while (depth + 1 > (int)maxColumnWidthByDepth.size())
			{
				maxColumnWidthByDepth.push_back(0);
				tableByDepth.push_back(TableList());
			}

			int& maxColumnWidthForDepth = maxColumnWidthByDepth.back();
			maxColumnWidthForDepth = Rococo::max(nameColumnWidth, maxColumnWidthForDepth);
			table.SetColumnWidth(0, maxColumnWidthForDepth);

			auto& tableList = tableByDepth.back();
			tableList.tables.push_back(&table);
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
				Strings::StackStringBuilder sb(message, sizeof(message));
				sb.AppendFormat("[PreviewData& data] had no fields.\n");

				PreviewData* pData = &data;
				while(pData)
				{
					sb.AppendFormat(" under node %s\n", pData->instanceName.length() > 0 ? pData->instanceName.c_str() : "<anon>");
					pData = pData->parent;
				}

				RaiseError(parentContainer.Panel(), EGRErrorCode::BadSpanHeight, __ROCOCO_FUNCTION__, "%s", message);
				return;
			}

			auto& collapser = CreateCollapser(parentContainer, *this);
			collapser.Panel().Set(spec.CollapserPadding).Add(EGRPanelFlags::AcceptsFocus);
			collapser.Panel().SetExpandToParentHorizontally();
			collapser.Panel().SetExpandToParentVertically();
			collapser.LeftSpacer().Panel().SetConstantWidth(depth * 24);
			collapser.LeftSpacer().SetTransparency(0.0f);

			MakeTransparent(collapser.Panel(), EGRSchemeColourSurface::CONTAINER_BACKGROUND);
			MakeTransparent(collapser.Panel(), EGRSchemeColourSurface::BUTTON);
			MakeTransparent(collapser.Panel(), EGRSchemeColourSurface::BUTTON_EDGE_BOTTOM_RIGHT);
			MakeTransparent(collapser.Panel(), EGRSchemeColourSurface::BUTTON_EDGE_TOP_LEFT);

			MakeTransparent(collapser.TitleBar().Panel(), EGRSchemeColourSurface::CONTAINER_TOP_LEFT);
			MakeTransparent(collapser.TitleBar().Panel(), EGRSchemeColourSurface::CONTAINER_BOTTOM_RIGHT);
			MakeTransparent(collapser.TitleBar().Panel(), EGRSchemeColourSurface::CONTAINER_BACKGROUND);

			GRAlignmentFlags alignment;
			alignment.Add(EGRAlignment::HCentre).Add(EGRAlignment::VCentre);
			collapser.CollapseButton().SetAlignment(alignment, spec.CollapserButtonSpacing);

			auto& titleDiv = collapser.TitleBar();
			titleDiv.Panel().SetConstantHeight(rowHeight);

			//parentContainer.Panel().GetColour(EGRSchemeColourSurface::)

			auto collapserSurface = ((depth % 2) == 0) ? EGRSchemeColourSurface::COLLAPSER_TITLE_DEPTH_EVEN : EGRSchemeColourSurface::COLLAPSER_TITLE_DEPTH_ODD;
			CopyAllColours(titleDiv.Panel(), titleDiv.Panel(), collapserSurface, EGRSchemeColourSurface::CONTAINER_BACKGROUND);
			
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

			auto& titleDescription = Rococo::Gui::CreateText(titleDiv.Widget()).SetText(title);
			titleDescription.Panel().SetExpandToParentVertically();
			titleDescription.Panel().SetExpandToParentHorizontally();
			titleDescription.Panel().Set(spec.TitleDescPadding);
			titleDescription.SetFont(spec.HeadingFontId);

			GRAlignmentFlags rightCentered;
			rightCentered.Add(EGRAlignment::Left).Add(EGRAlignment::VCentre);

			titleDescription.SetAlignment(rightCentered, { 4,0 });

			MakeTransparent(titleDescription.Panel(), EGRSchemeColourSurface::LABEL_BACKGROUND);
			MakeTransparent(titleDescription.Panel(), EGRSchemeColourSurface::CONTAINER_BOTTOM_RIGHT);
			MakeTransparent(titleDescription.Panel(), EGRSchemeColourSurface::CONTAINER_TOP_LEFT);

			titleDescription.SetTextColourSurface(EGRSchemeColourSurface::COLLAPSER_TITLE_TEXT);
			titleDescription.SetTextColourShadowSurface(EGRSchemeColourSurface::COLLAPSER_TITLE_SHADOW);
			
			auto& list = CreateVerticalList(collapser.ClientArea().Widget());
			list.Panel().SetExpandToParentHorizontally();
			list.Panel().SetExpandToParentVertically();
			list.Panel().SetLayoutDirection(ELayoutDirection::TopToBottom);


			int32 firstSimpleFieldIndex = -1;
			int32 nextSimpleFieldIndex = -1;

			for (int32 i = 0; i < (int32)data.fields.size(); ++i)
			{
				auto& f = *data.fields[i];

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

					AddSubObject(*data.fields[i], list.Widget(), depth + 1);
				}
			}

			if (firstSimpleFieldIndex >= 0)
			{
				AddFieldTable(data, firstSimpleFieldIndex, (int32)data.fields.size() - 1, list.Widget(), depth);
			}
		}

		IGRWidgetViewport* viewport = nullptr;

		IGRWidgetViewport& Viewport() override
		{
			if (!viewport)
			{
				viewport = &CreateViewportWidget(*this);
				viewport->SetLineDeltaPixels(spec.LineDeltaPixels);

				auto& vp = viewport->Widget().Panel();
				vp.SetExpandToParentHorizontally();
				vp.SetExpandToParentVertically();
				vp.SetDesc("PropEditor-Viewport");
			}

			return *viewport;
		}

		int rowHeight = 30;

		void SetRowHeight(int height) override
		{
			rowHeight = clamp(height, 16, 300);
		}

		IReflectionVisitation* currentVisitation = nullptr;

		void View(IReflectionVisitation* visitation) override
		{
			if (!visitation || !visitation->AcceptVisitor(previewer))
			{
				return;
			}

			auto* nextTarget = &visitation->Target();
			if (currentVisitation && &currentVisitation->Target() != nextTarget)
			{
				visitation->OnVisitorGone(previewer);
			}

			currentVisitation = visitation;

			editorToPreviewField.clear();

			currentVisitation->Target().Visit(previewer);

			auto& viewport = Viewport();
			auto* node = previewer.root;

			if (node) SyncUIToPreviewerRecursive(*node, viewport.ClientArea().Widget(), 0);

			if (firstCreatedEditBox != lastCreatedEditBox)
			{
				firstCreatedEditBox->Panel().Set(EGRNavigationDirection::Up, lastCreatedEditBox->Panel().Desc());
				lastCreatedEditBox->Panel().Set(EGRNavigationDirection::Down, firstCreatedEditBox->Panel().Desc());
			}

			for (size_t depth = 0; depth < tableByDepth.size(); depth++)
			{
				auto& tablesAtDepth = tableByDepth[depth];
				int maxWidth = maxColumnWidthByDepth[depth];
				
				for (auto& t : tablesAtDepth.tables)
				{
					t->SetColumnWidth(0, maxWidth);
				}
			}

			SetCollapserSizes();
		}

		int ComputeAndAssignCollapserHeights(IGRWidgetCollapser& collapserParent)
		{
			int32 heightOfDescendants = collapserParent.TitleBar().Panel().Span().y;

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
							heightOfDescendants += table->EstimateHeight();
						}
					}
				}
			}

			auto& cp = collapserParent.Panel();
			cp.SetConstantHeight(heightOfDescendants);
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

	void Previewer::CancelVisit(IReflectionVisitation& visitation)
	{
		if (owner.CancelVisit(visitation))
		{
			root->CancelVisitRecursive();
		}
	}
}

namespace Rococo::Gui
{
	ROCOCO_GUI_RETAINED_API cstr IGRWidgetPropertyEditorTree::InterfaceId()
	{
		return "IGRWidgetPropertyEditorTree";
	}

	ROCOCO_GUI_RETAINED_API IGRWidgetPropertyEditorTree& CreatePropertyEditorTree(IGRWidget& parent, IGRPropertyEditorPopulationEvents& populationEventHandler, const PropertyEditorSpec& spec)
	{
		auto& gr = parent.Panel().Root().GR();

		struct GRPropertyEditorTreeFactory : IGRWidgetFactory
		{
			IGRPropertyEditorPopulationEvents& populationEventHandler;
			const PropertyEditorSpec& spec;

			GRPropertyEditorTreeFactory(IGRPropertyEditorPopulationEvents& _populationEventHandler, const PropertyEditorSpec& _spec):
				populationEventHandler(_populationEventHandler),
				spec(_spec)

			{

			}

			IGRWidget& CreateWidget(IGRPanel& panel)
			{
				return *new GRANON::GRPropertyEditorTree(panel, populationEventHandler, spec);
			}
		};

		GRPropertyEditorTreeFactory factory(populationEventHandler, spec);
		auto* tree = static_cast<GRANON::GRPropertyEditorTree*>(Cast<IGRWidgetPropertyEditorTree>(gr.AddWidget(parent.Panel(), factory)));
		return *tree;
	}
}