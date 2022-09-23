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
			for (auto& f : data.fields)
			{
				auto& div = CreateDivision(list);
				div.Panel().Resize({ 30, 30 });

				GRAlignmentFlags buttonAlignment;
				buttonAlignment.Add(GRAlignment::VCentre).Add(GRAlignment::Left);
				auto& button = CreateButton(div).SetTitle(f.fieldName.c_str()).SetAlignment(buttonAlignment, {2,2});
				button.Panel().Resize({ 100, 24 }).Add(GRAnchors::TopAndBottom()).Add(GRAnchors::ExpandVertically());

				switch (f.value.type)
				{
				case PrimitiveType::I32:
				{										
				}
				default:
					break;
				}
			}
		}

		void SyncUIToPreviewer(IGuiRetained& gr) override
		{
			auto* frame = gr.TryGetFrame(IdWidget{ "MPlat-MainFrame" });
			if (!frame) return;

			ClearFrame(*frame);

			auto& clientArea = frame->ClientArea();

			auto& list = CreateVerticalList(clientArea);
			list.Panel().Resize({ 240, 0 }).Add(GRAnchors::Left()).Add(GRAnchors::TopAndBottom()).Add(GRAnchors::ExpandVertically());

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
