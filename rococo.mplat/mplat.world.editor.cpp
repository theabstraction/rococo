#include <rococo.gui.retained.h>
#include "mplat.editor.h"
#include <rococo.reflector.h>
#include <rococo.strings.h>
#include <vector>

using namespace Rococo;
using namespace Rococo::Gui;
using namespace Rococo::MPEditor;
using namespace Rococo::Reflection;
using namespace Rococo::Strings;

namespace ANON
{
	void BuildMenus(IGRMainFrame& frame)
	{
		auto& menu = frame.MenuBar();
		menu.Panel().Set(ESchemeColourSurface::MENU_BUTTON_EDGE_BOTTOM_RIGHT, RGBAb(0, 0, 0, 0)).Set(ESchemeColourSurface::MENU_BUTTON_EDGE_TOP_LEFT, RGBAb(0, 0, 0, 0));

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

		auto& titleBar = *frame.MenuBar().Panel().Parent();
		titleBar.Set(ESchemeColourSurface::CONTAINER_BACKGROUND, RGBAb(0, 0, 0, 255)).Set(ESchemeColourSurface::CONTAINER_BACKGROUND_HOVERED, RGBAb(8, 8, 8, 255));
		titleBar.Set(ESchemeColourSurface::MENU_BUTTON_RAISED, RGBAb(0, 0, 0, 255)).Set(ESchemeColourSurface::MENU_BUTTON_PRESSED, RGBAb(8, 8, 8, 255));
		titleBar.Set(ESchemeColourSurface::BUTTON_RAISED, RGBAb(0, 0, 0, 255)).Set(ESchemeColourSurface::BUTTON_PRESSED, RGBAb(24, 24, 24, 255));
		titleBar.Set(ESchemeColourSurface::BUTTON_RAISED_AND_HOVERED, RGBAb(32, 32, 32, 255)).Set(ESchemeColourSurface::BUTTON_PRESSED_AND_HOVERED, RGBAb(48, 48, 48, 255));
		titleBar.Set(ESchemeColourSurface::MENU_BUTTON_RAISED_AND_HOVERED, RGBAb(16, 16, 16, 255)).Set(ESchemeColourSurface::MENU_BUTTON_PRESSED_AND_HOVERED, RGBAb(32, 32, 32, 255));
		titleBar.Set(ESchemeColourSurface::IMAGE_FOG_HOVERED, RGBAb(0, 0, 0, 64)).Set(ESchemeColourSurface::IMAGE_FOG, RGBAb(0, 0, 0, 128));
	}

	void BuildUpperRightToolbar(IGRMainFrame& frame)
	{
		auto& tools = frame.TopRightHandSideTools();
		tools.SetChildAlignment(GRAlignment::Right);
		
		CreateButton(tools).SetTitle("Min").SetImagePath("!textures/toolbars/3rd-party/www.aha-soft.com/Down.tiff");
		CreateButton(tools).SetTitle("Max").SetImagePath("!textures/toolbars/3rd-party/www.aha-soft.com/Expand.tiff");
		CreateButton(tools).SetTitle("Close").SetImagePath("!textures/toolbars/3rd-party/www.aha-soft.com/Close.tiff");

		tools.ResizeToFitChildren();
	}
	
	struct PreviewData;

	enum class PrimitiveType
	{
		I32, I64, F32, F64, BOOL, OBJECT
	};

	union PreviewPrimitive
	{
		int32 int32Value;
		int64 int64Value;
		float float32Value;
		double float64Value;
		bool boolValue;
		PreviewData* pObject;
	};

	struct PrimitiveVariant
	{
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
		case PrimitiveType::OBJECT:
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
				if (i.value.type == PrimitiveType::OBJECT)
				{
					auto* subObject = i.value.primitive.pObject;
					delete subObject;
				}
			}
		}

		PreviewData* parent;
		HString sectionName;
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

		void Reflect(cstr name, IReflectionVisitor& subTarget, ReflectionMetaData& metaData) override
		{
			
		}

		void SetSection(cstr sectionName)
		{
			target->sectionName = sectionName;
		}
	};

	struct MPlatEditor : IMPEditorSupervisor
	{
		IGuiRetained& gr;
		bool isVisible = false;

		MPlat_Reflection_Previewer previewer;

		MPlatEditor(IGuiRetained& _gr): gr(_gr)
		{

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

		void ClearFrame(IGRMainFrame& frame)
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

		void InstantiateUI()
		{
			auto& frame = gr.BindFrame(IdWidget{ "MPlat-MainFrame" });

			auto& scheme = gr.Root().Scheme();
			SetSchemeColours_ThemeGrey(scheme);
			BuildMenus(frame);
			BuildUpperRightToolbar(frame);
		}

		void SyncUIToPreviewerRecursive(PreviewData& data, IGuiRetained& gr, IGRWidgetVerticalList& list, int32 depth)
		{
			list.Panel().Set(ESchemeColourSurface::CONTAINER_BACKGROUND, RGBAb(48, 0, 0, 255));
			list.Panel().Set(ESchemeColourSurface::CONTAINER_BACKGROUND_HOVERED, RGBAb(50, 0, 0, 255));

			for (auto& f : data.fields)
			{
				auto& div = CreateDivision(list);
				div.Panel().Resize({ 30, 30 });				

				GRAlignmentFlags nameAlignment;
				nameAlignment.Add(GRAlignment::VCentre).Add(GRAlignment::Left);
				auto& nameText = CreateText(div).SetText(f.fieldName.c_str()).SetAlignment(nameAlignment, {2,2});
				nameText.Panel().Resize({ 100, 24 }).Add(GRAnchors::TopAndBottom()).Add(GRAnchors::ExpandVertically()).Add(GRAnchors::Left()).Add(GRAnchors::ExpandHorizontally()).Set(GRAnchorPadding{ 4, 0, 0, 0 });

				GRAlignmentFlags valueAlignment;
				valueAlignment.Add(GRAlignment::VCentre).Add(GRAlignment::Left);
				auto& valueText = CreateEditBox(div).SetAlignment(valueAlignment, { 2,2 });
				valueText.Panel().SetParentOffset({ 100, 24 }).Add(GRAnchors::TopAndBottom()).Add(GRAnchors::ExpandVertically()).Add(GRAnchors::Right()).Add(GRAnchors::ExpandHorizontally()).Set(GRAnchorPadding { 0, 4, 0, 0});

				char buf[16];
				ToAscii(f.value, buf, sizeof buf);
				valueText.SetText(buf);
			}
		}

		void SyncUIToPreviewer(IGuiRetained& gr) override
		{
			auto* frame = gr.FindFrame(IdWidget{ "MPlat-MainFrame" });
			if (!frame) return;

			ClearFrame(*frame);

			frame->ClientArea().Panel().Set(ESchemeColourSurface::FOCUSED_EDITOR, RGBAb(0, 0, 0, 255));
			frame->ClientArea().Panel().Set(ESchemeColourSurface::FOCUSED_EDITOR_HOVERED, RGBAb(16, 16, 16, 255));

			auto& listCollapser = CreateCollapser(frame->ClientArea());
			auto& list = CreateVerticalList(listCollapser.ClientArea());
			listCollapser.Panel().Resize({ 240, 0 }).Add(GRAnchors::Left()).Add(GRAnchors::TopAndBottom()).Add(GRAnchors::ExpandVertically());
			list.Panel().Add(GRAnchors::LeftAndRight()).Add(GRAnchors::TopAndBottom()).Add(GRAnchors::ExpandVertically()).Add(GRAnchors::ExpandHorizontally());
			listCollapser.Panel().Set(ESchemeColourSurface::TEXT, RGBAb(224, 224, 224, 255)).Set(ESchemeColourSurface::TEXT_HOVERED, RGBAb(255, 255, 255, 255));
			listCollapser.Panel().Set(ESchemeColourSurface::EDIT_TEXT, RGBAb(224, 224, 224, 255)).Set(ESchemeColourSurface::EDIT_TEXT_HOVERED, RGBAb(255, 255, 255, 255));

			auto* node = previewer.root;
			if (node) SyncUIToPreviewerRecursive(*node, gr, list, 0);
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
