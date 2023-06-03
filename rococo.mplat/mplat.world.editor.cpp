#include <rococo.gui.retained.ex.h>
#include "mplat.editor.h"
#include <rococo.reflector.h>
#include <rococo.strings.h>
#include <vector>
#include <rococo.mplat.h>
#include <rococo.task.queue.h>

using namespace Rococo;
using namespace Rococo::Gui;
using namespace Rococo::MPEditor;
using namespace Rococo::Reflection;
using namespace Rococo::Strings;

namespace ANON
{
	void BuildMenus(IGRWidgetMainFrame& frame)
	{
		auto& menu = frame.MenuBar();
		menu.Widget().Panel().
			Set(ESchemeColourSurface::MENU_BUTTON_EDGE_BOTTOM_RIGHT, RGBAb(0, 0, 0, 0)).
			Set(ESchemeColourSurface::MENU_BUTTON_EDGE_TOP_LEFT, RGBAb(0, 0, 0, 0));

		auto fileMenu = menu.AddSubMenu(GRMenuItemId::Root(), GRMenuSubMenu("File"));
		auto editMenu = menu.AddSubMenu(GRMenuItemId::Root(), GRMenuSubMenu("Edit"));
		auto viewMenu = menu.AddSubMenu(GRMenuItemId::Root(), GRMenuSubMenu("View"));
		auto projectMenu = menu.AddSubMenu(GRMenuItemId::Root(), GRMenuSubMenu("Project"));
		auto windowMenu = menu.AddSubMenu(GRMenuItemId::Root(), GRMenuSubMenu("Window"));
		auto helpMenu = menu.AddSubMenu(GRMenuItemId::Root(), GRMenuSubMenu("Help"));

		menu.AddButton(fileMenu, { "New", { 0, nullptr } });
		menu.AddButton(fileMenu, { "Open...", { 0, nullptr } });
		menu.AddButton(fileMenu, { "Save", { 0, nullptr } });
		menu.AddButton(fileMenu, { "Save As...", { 0, nullptr } });
		menu.AddButton(fileMenu, { "Exit", { 0, nullptr } });

		menu.AddButton(editMenu, { "Find...", { 0, nullptr } });
		menu.AddButton(editMenu, { "Replace...", { 0, nullptr } });
		menu.AddButton(editMenu, { "Copy", { 0, nullptr } });
		menu.AddButton(editMenu, { "Cut", { 0, nullptr } });
		menu.AddButton(editMenu, { "Paste", { 0, nullptr } });

		menu.AddButton(viewMenu, { "Solution", { 0, nullptr } });
		menu.AddButton(viewMenu, { "Classes", { 0, nullptr } });
		menu.AddButton(viewMenu, { "Repo", { 0, nullptr } });
		menu.AddButton(viewMenu, { "Debugger", { 0, nullptr } });
		menu.AddButton(viewMenu, { "Output", { 0, nullptr } });

		menu.AddButton(projectMenu, { "Build", { 0, nullptr } });
		menu.AddButton(projectMenu, { "Rebuild", { 0, nullptr } });
		menu.AddButton(projectMenu, { "Debug", { 0, nullptr } });
		menu.AddButton(projectMenu, { "Cancel", { 0, nullptr } });

		menu.AddButton(windowMenu, { "Split", { 0, nullptr } });
		menu.AddButton(windowMenu, { "Cascade", { 0, nullptr } });
		menu.AddButton(windowMenu, { "Merge", { 0, nullptr } });

		auto toggles = menu.AddSubMenu(windowMenu, GRMenuSubMenu("Toggles"));
		menu.AddButton(toggles, { "Toolkit", { 0, nullptr } });
		menu.AddButton(toggles, { "Properties", { 0, nullptr } });
		menu.AddButton(toggles, { "Log", { 0, nullptr } });
		menu.AddButton(windowMenu, { "Close All", { 0, nullptr } });

		menu.AddButton(helpMenu, { "About...", { 0, nullptr } });
		menu.AddButton(helpMenu, { "Check for updates", { 0, nullptr } });
		menu.AddButton(helpMenu, { "Version", { 0, nullptr } });
		menu.AddButton(helpMenu, { "Purchase License", { 0, nullptr } });

		auto& titleBar = *frame.MenuBar().Widget().Panel().Parent();
		titleBar.Set(ESchemeColourSurface::CONTAINER_BACKGROUND, RGBAb(0, 0, 0, 255)).Set(ESchemeColourSurface::CONTAINER_BACKGROUND_HOVERED, RGBAb(8, 8, 8, 255));
		titleBar.Set(ESchemeColourSurface::MENU_BUTTON_RAISED, RGBAb(0, 0, 0, 255)).Set(ESchemeColourSurface::MENU_BUTTON_PRESSED, RGBAb(8, 8, 8, 255));
		titleBar.Set(ESchemeColourSurface::BUTTON_RAISED, RGBAb(0, 0, 0, 255)).Set(ESchemeColourSurface::BUTTON_PRESSED, RGBAb(24, 24, 24, 255));
		titleBar.Set(ESchemeColourSurface::BUTTON_RAISED_AND_HOVERED, RGBAb(32, 32, 32, 255)).Set(ESchemeColourSurface::BUTTON_PRESSED_AND_HOVERED, RGBAb(48, 48, 48, 255));
		titleBar.Set(ESchemeColourSurface::MENU_BUTTON_RAISED_AND_HOVERED, RGBAb(16, 16, 16, 255)).Set(ESchemeColourSurface::MENU_BUTTON_PRESSED_AND_HOVERED, RGBAb(32, 32, 32, 255));
		titleBar.Set(ESchemeColourSurface::IMAGE_FOG_HOVERED, RGBAb(0, 0, 0, 64)).Set(ESchemeColourSurface::IMAGE_FOG, RGBAb(0, 0, 0, 128));
	}

	enum { TOOLBAR_EVENT_MINIMIZE = 40001, TOOLBAR_EVENT_RESTORE, TOOLBAR_EVENT_EXIT };

	void BuildUpperRightToolbar(IGRWidgetMainFrame& frame)
	{
		auto& tools = frame.TopRightHandSideTools();
		tools.SetChildAlignment(GRAlignment::Right);
		
		auto& minimizer = CreateButton(tools.Widget()).SetTitle("Min").SetImagePath("!textures/toolbars/3rd-party/www.aha-soft.com/Down.tiff").SetClickCriterion(GRClickCriterion::OnDownThenUp).SetEventPolicy(GREventPolicy::PublicEvent);
		auto& restorer = CreateButton(tools.Widget()).SetTitle("Max").SetImagePath("!textures/toolbars/3rd-party/www.aha-soft.com/Expand.tiff").SetClickCriterion(GRClickCriterion::OnDownThenUp).SetEventPolicy(GREventPolicy::PublicEvent);
		auto& closer = CreateButton(tools.Widget()).SetTitle("Close").SetImagePath("!textures/toolbars/3rd-party/www.aha-soft.com/Close.tiff").SetClickCriterion(GRClickCriterion::OnDownThenUp).SetEventPolicy(GREventPolicy::PublicEvent);

		minimizer.SetMetaData({ TOOLBAR_EVENT_MINIMIZE, "OnMinimize" });
		restorer.SetMetaData({ TOOLBAR_EVENT_RESTORE, "OnMinimize" });
		closer.SetMetaData({ TOOLBAR_EVENT_EXIT, "OnExit" });

		tools.ResizeToFitChildren();
	}
	
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
		PreviewData(PreviewData* _parent): parent(_parent)
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

	struct MPlat_Reflection_Previewer : IReflectionVisitor
	{
		PreviewData* root = nullptr;
		PreviewData* target = nullptr;

		MPlat_Reflection_Previewer()
		{
			root = new PreviewData(nullptr);
			target = root;
		}

		~MPlat_Reflection_Previewer()
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

	struct MPlatEditor : IMPEditorSupervisor, IGREventHandler
	{
		IGuiRetained& gr;
		Platform* platform = nullptr;
		bool isVisible = false;

		MPlat_Reflection_Previewer previewer;

		MPlatEditor(IGuiRetained& _gr): gr(_gr)
		{
			static_cast<IGuiRetainedSupervisor&>(gr).SetEventHandler(this);
		}

		void SetPlatform(Platform* platform)
		{
			this->platform = platform;
		}

		void OnButtonClickTaskResult(int64 code)
		{
			switch (code)
			{
			case TOOLBAR_EVENT_MINIMIZE:
				platform->renderer.SwitchToWindowMode();
				Rococo::Windows::MinimizeApp(platform->mainWindow);
				break;
			case TOOLBAR_EVENT_RESTORE:
				if (platform->renderer.IsFullscreen())
				{
					platform->renderer.SwitchToWindowMode();
				}
				else
				{
					platform->renderer.SwitchToFullscreen();
				}
				break;
			case TOOLBAR_EVENT_EXIT:
				if (platform->renderer.IsFullscreen())
				{
					platform->renderer.SwitchToWindowMode();
				}

				Rococo::Windows::SendCloseEvent(platform->mainWindow);
				break;
			}
		}

		void OnButtonClick(WidgetEvent& buttonEvent)
		{
			int64 id = buttonEvent.iMetaData;
			switch (id)
			{
			case TOOLBAR_EVENT_MINIMIZE:
				platform->renderer.SwitchToWindowMode();
				Rococo::Windows::MinimizeApp(platform->mainWindow);
				break;
			case TOOLBAR_EVENT_RESTORE:
				if (platform->renderer.IsFullscreen())
				{
					platform->renderer.SwitchToWindowMode();
				}
				else
				{
					platform->renderer.SwitchToFullscreen();
				}
				break;
			case TOOLBAR_EVENT_EXIT:
				if (platform->renderer.IsFullscreen())
				{
					platform->renderer.SwitchToWindowMode();
				}

				Rococo::Windows::SendCloseEvent(platform->mainWindow);
				break;
			}
		}

		EventRouting OnGREvent(WidgetEvent& ev) override
		{
			switch (ev.eventType)
			{
			case WidgetEventType::BUTTON_CLICK:
				OnButtonClick(ev);
				break;
			}
			return EventRouting::Terminate;
		}

		void Free() override
		{
			delete this;
		}

		bool IsVisible() const override
		{
			return isVisible;
		}

		void SetVisibility(bool isVisible) override
		{
			this->isVisible = isVisible;

			if (isVisible)
			{
				InstantiateUI();
				gr.SetVisible(true);
			}
			else
			{
				gr.SetVisible(false);
			}
		}

		void ClearFrame(IGRWidgetMainFrame& frame)
		{
			struct : IEventCallback<IGRPanel>
			{
				void OnEvent(IGRPanel& panel) override
				{
					panel.MarkForDelete();
				}
			} cb;
			frame.ClientArea().Panel().EnumerateChildren(&cb);
			frame.ClientArea().Panel().Root().GR().GarbageCollect();
		}

		IdWidget ID_EDITOR_FRAME = { "MPlat-MainFrame" };

		void InstantiateUI()
		{
			auto& frame = gr.BindFrame(ID_EDITOR_FRAME);
			auto& scheme = gr.Root().Scheme();
			SetSchemeColours_ThemeGrey(scheme);
			BuildMenus(frame);
			BuildUpperRightToolbar(frame);

			auto& custodian = gr.Root().Custodian();
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
		void AddFieldTable(PreviewData& data, IGuiRetained& gr, int32 firstValidIndex, int32 lastValidIndex, IGRWidget& parent, int depth, int& accumulatedHeight)
		{
			auto& table = CreateTable(parent);
			table.Widget().Panel().Set(GRAnchors::ExpandAll());

			GRColumnSpec nameSpec;
			nameSpec.name = "Name";
			nameSpec.maxWidth = 240;
			nameSpec.minWidth = 64;
			nameSpec.defaultWidth = 60;
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

		void AddSubObject(PreviewField& subObjectField, IGuiRetained& gr, IGRWidget& parent, int depth, int& accumulatedHeight)
		{
			SyncUIToPreviewerRecursive(*subObjectField.value.primitive.pSubObject, gr, parent, depth, accumulatedHeight);
		}

		void SyncUIToPreviewerRecursive(PreviewData& data, IGuiRetained& gr, IGRWidget& parentContainer, int32 depth, int& accumulatedParentHeight)
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

			titleDescription.SetAlignment(rightCentered, {0,0});

			auto& list = CreateVerticalList(collapser.ClientArea());
			list.Panel().Set(GRAnchors::ExpandAll());

			int32 firstSimpleFieldIndex = -1;
			int32 nextSimpleFieldIndex = -1;

			int32 accumulatedCollapserAreaHeight = 30;

			for (int32 i = 0; i < (int32) data.fields.size(); ++i)
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
						AddFieldTable(data, gr, firstSimpleFieldIndex, nextSimpleFieldIndex, list, depth, accumulatedCollapserAreaHeight);
						firstSimpleFieldIndex = -1;
						nextSimpleFieldIndex = -1;
					}

					AddSubObject(data.fields[i], gr, list, depth + 1, accumulatedCollapserAreaHeight);
				}
			}

			if (firstSimpleFieldIndex >= 0)
			{
				AddFieldTable(data, gr, firstSimpleFieldIndex, (int32) data.fields.size() - 1, list, depth, accumulatedCollapserAreaHeight);
			}

			collapser.Widget().Panel().Resize({ 0, accumulatedCollapserAreaHeight });

			accumulatedParentHeight += accumulatedCollapserAreaHeight;
		}

		void SyncUIToPreviewer(IGuiRetained& gr) override
		{
			auto* frame = gr.FindFrame(ID_EDITOR_FRAME);
			if (!frame) Throw(0, "%s: Unexpected missing frame. gr.FindFrame(ID_EDITOR_FRAME) returned null", __FUNCTION__);

			ClearFrame(*frame);

			auto& framePanel = frame->ClientArea().Panel();

			framePanel.Set(ESchemeColourSurface::TEXT, RGBAb(224, 224, 224, 255)).Set(ESchemeColourSurface::TEXT_HOVERED, RGBAb(255, 255, 255, 255));
			framePanel.Set(ESchemeColourSurface::EDIT_TEXT, RGBAb(224, 224, 224, 255)).Set(ESchemeColourSurface::EDIT_TEXT_HOVERED, RGBAb(255, 255, 255, 255));
			framePanel.Set(ESchemeColourSurface::FOCUSED_EDITOR, RGBAb(0, 0, 0, 255));
			framePanel.Set(ESchemeColourSurface::FOCUSED_EDITOR_HOVERED, RGBAb(16, 16, 16, 255));
			framePanel.Set(ESchemeColourSurface::SCROLLER_BUTTON_BACKGROUND, RGBAb(64, 64, 64, 255)).Set(ESchemeColourSurface::SCROLLER_BUTTON_BACKGROUND_HOVERED, RGBAb(96, 96, 96, 255));
			framePanel.Set(ESchemeColourSurface::SCROLLER_BUTTON_TOP_LEFT, RGBAb(192, 192, 192, 255)).Set(ESchemeColourSurface::SCROLLER_BUTTON_TOP_LEFT_HOVERED, RGBAb(255, 255, 255, 255));
			framePanel.Set(ESchemeColourSurface::SCROLLER_BUTTON_BOTTOM_RIGHT, RGBAb(128, 128, 128, 255)).Set(ESchemeColourSurface::SCROLLER_BUTTON_BOTTOM_RIGHT_HOVERED, RGBAb(192, 192, 192, 255));
			framePanel.Set(ESchemeColourSurface::SCROLLER_BAR_BACKGROUND, RGBAb(64, 64, 64, 255)).Set(ESchemeColourSurface::SCROLLER_BAR_BACKGROUND_HOVERED, RGBAb(72, 72, 72, 255));
			framePanel.Set(ESchemeColourSurface::SCROLLER_BAR_TOP_LEFT, RGBAb(128, 128, 128, 255)).Set(ESchemeColourSurface::SCROLLER_BAR_TOP_LEFT_HOVERED, RGBAb(136, 136, 136, 255));
			framePanel.Set(ESchemeColourSurface::SCROLLER_BAR_BOTTOM_RIGHT, RGBAb(96, 96, 96, 255)).Set(ESchemeColourSurface::SCROLLER_BAR_BOTTOM_RIGHT_HOVERED, RGBAb(160, 160, 160, 255));
			framePanel.Set(ESchemeColourSurface::SCROLLER_SLIDER_BACKGROUND, RGBAb(64, 64, 64, 255)).Set(ESchemeColourSurface::SCROLLER_SLIDER_BACKGROUND_HOVERED, RGBAb(192, 192, 192, 255));
			framePanel.Set(ESchemeColourSurface::SCROLLER_SLIDER_TOP_LEFT, RGBAb(128, 128, 128, 255)).Set(ESchemeColourSurface::SCROLLER_SLIDER_TOP_LEFT_HOVERED, RGBAb(255, 255, 255, 255));
			framePanel.Set(ESchemeColourSurface::SCROLLER_SLIDER_BOTTOM_RIGHT, RGBAb(96, 96, 96, 255)).Set(ESchemeColourSurface::SCROLLER_SLIDER_BOTTOM_RIGHT_HOVERED, RGBAb(224, 224, 224, 255));
			framePanel.Set(ESchemeColourSurface::SCROLLER_TRIANGLE_NORMAL, RGBAb(128, 128, 128, 255)).Set(ESchemeColourSurface::SCROLLER_TRIANGLE_HOVERED, RGBAb(192, 192, 192, 255));

			auto& frameSplitter = CreateLeftToRightSplitter(frame->ClientArea(), 240, false).SetDraggerMinMax(240, 8192);
			frameSplitter.Widget().Panel().Add(GRAnchors::ExpandAll());

			auto& viewport = CreateViewportWidget(frameSplitter.First());

			GRAnchors anchors = anchors.ExpandAll();

			viewport.Widget().Panel().Set(anchors);

			auto* node = previewer.root;

			int32 accumulatedHeight = 0;
			if (node) SyncUIToPreviewerRecursive(*node, gr, viewport.ClientArea(), 0, accumulatedHeight);
		}

		void Preview(IReflectionTarget& target) override
		{
			target.Visit(previewer);
		}
	};
}

namespace Rococo::MPEditor
{
	IMPEditorSupervisor* CreateMPlatEditor(Rococo::Gui::IGuiRetained& gr)
	{
		return new ANON::MPlatEditor(gr);
	}
}
