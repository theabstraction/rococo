#include "dystopia.h"
#include <rococo.renderer.h>
#include <rococo.io.h>
#include <vector>
#include "meshes.h"
#include "human.types.h"
#include "rococo.ui.h"

using namespace Dystopia;
using namespace Rococo;
using namespace Rococo::Fonts;

namespace Dystopia
{
}

namespace
{
	void DrawTestTriangles(IGuiRenderContext& rc)
	{
		GuiVertex q0[6] =
		{
			{ 0.0f,600.0f, 1.0f, 0.0f,{ 255,0,0,255 }, 0.0f, 0.0f, 0 }, // bottom left
			{ 800.0f,600.0f, 1.0f, 0.0f,{ 0,255,0,255 }, 0.0f, 0.0f, 0 }, // bottom right
			{ 0.0f,  0.0f, 1.0f, 0.0f,{ 0,0,255,255 }, 0.0f, 0.0f, 0 }, // top left
			{ 0.0f,  0.0f, 1.0f, 0.0f,{ 255,255,0,255 }, 0.0f, 0.0f, 0 }, // top left
			{ 800.0f,600.0f, 1.0f, 0.0f,{ 0,255,255,255 }, 0.0f, 0.0f, 0 }, // bottom right
			{ 800.0f,  0.0f, 1.0f, 0.0f,{ 255,0,255,255 }, 0.0f, 0.0f, 0 }  // top right
		};

		rc.AddTriangle(q0);
		rc.AddTriangle(q0 + 3);
	}

	void DrawGuiMetrics(IGuiRenderContext& rc)
	{
		GuiMetrics metrics;
		rc.Renderer().GetGuiMetrics(metrics);

		wchar_t info[256];
		SafeFormat(info, _TRUNCATE, L"Mouse: (%d,%d). Screen(%d,%d)", metrics.cursorPosition.x, metrics.cursorPosition.y, metrics.screenSpan.x, metrics.screenSpan.y);

		Graphics::RenderHorizontalCentredText(rc, info, RGBAb{ 255, 255, 255, 255 }, 1, Vec2i{ 25, 25 });
	}

	void DrawVector(IGuiRenderContext& grc, const Vec4& v, const Vec2i& pos)
	{
		wchar_t info[256];
		SafeFormat(info, _TRUNCATE, L"(%f) (%f) (%f) (%f)\n", v.x, v.y, v.z, v.w);

		Graphics::RenderHorizontalCentredText(grc, info, RGBAb{ 255, 255, 255, 255 }, 3, pos);
	}

	void DrawVector(IGuiRenderContext& grc, const Vec3& v, const Vec2i& pos)
	{
		wchar_t info[256];
		SafeFormat(info, _TRUNCATE, L"(%f) (%f) (%f) ", v.x, v.y, v.z);

		Graphics::RenderHorizontalCentredText(grc, info, RGBAb{ 255, 255, 255, 255 }, 3, pos);
	}

	struct HumanFactory: public IHumanFactory
	{
		ILevel* level;
		IIntent* controls;

		virtual IHumanAISupervisor* CreateHuman(ID_ENTITY id, IInventory& inventory, HumanType typeId)
		{
			switch (typeId)
			{
			case HumanType_Bobby:
				return CreateBobby(id, inventory, *level);
			case HumanType_Vigilante:
				return CreateVigilante(id, *controls, *level);
			}
			return nullptr;
		}
	};

	class DystopiaApp : public IApp, public IEventCallback<GuiEventArgs>, public IUIPaneFactory
	{
	private:
		AutoFree<Post::IPostboxSupervisor> postbox;
		AutoFree<IUIStackSupervisor> uiStack;
		AutoFree<IGuiSupervisor> gui;
		AutoFree<IDebuggerWindow> debuggerWindow;
		AutoFree<ISourceCache> sourceCache;
		AutoFree<IMeshLoader> meshes;
		
		Environment e;
		HumanFactory humanFactory;
		AutoFree<ILevelSupervisor> level;
		AutoFree<ILevelLoader> levelLoader;
		
		AutoFree<IUIControlPane> isometricGameWorldView;
		AutoFree<IUIPaneSupervisor> guiWorldInterface;
		AutoFree<IUIPaneSupervisor> inventoryPaneSelf;
	
	public:
		DystopiaApp(IRenderer& _renderer, IInstallation& _installation) :
			postbox(Post::CreatePostbox()),
			uiStack(CreateUIStack(*postbox)),
			gui(CreateGui(e, *uiStack)),
			debuggerWindow(CreateDebuggerWindow(&_renderer.Window())),
			sourceCache(CreateSourceCache(_installation)),
			meshes(CreateMeshLoader(_installation, _renderer, *sourceCache)),		
			e{ _installation, _renderer, *debuggerWindow, *sourceCache, *meshes, *gui, *uiStack, *postbox},
			level(CreateLevel(e, humanFactory)),
			levelLoader(CreateLevelLoader(e, *level)),
			isometricGameWorldView(CreatePaneIsometric(e, *level)),
			guiWorldInterface(CreateGuiWorldInterface(e,*level)),
			inventoryPaneSelf(CreateInventoryPane(e, *level))
		{
			uiStack->SetFactory(*this);
			humanFactory.level = level;
			humanFactory.controls = isometricGameWorldView->PlayerIntent();

			gui->SetEventHandler(this);
		}
		
		~DystopiaApp()
		{
		}

		virtual void FreeInstance(ID_PANE id, IUIPaneSupervisor* pane)
		{
			switch (id)
			{
			case ID_PANE_GENERIC_CONTEXT_MENU:
			case ID_PANE_GENERIC_DIALOG_BOX:
				pane->Free();
			}
		}

		virtual IUIPaneSupervisor* GetOrCreatePane(ID_PANE id)
		{
			switch (id)
			{
			case ID_PANE_ISOMETRIC_GAME_VIEW:
				return isometricGameWorldView;
			case ID_PANE_GUI_WORLD_INTERFACE:
				return guiWorldInterface;
			case ID_PANE_INVENTORY_SELF:
				return inventoryPaneSelf;
			default:
				return nullptr;
			}
		}

		virtual void OnEvent(GuiEventArgs& args)
		{

		}

		virtual void Free()
		{ 
			delete this;
		}

		virtual void OnCreated()
		{
			uiStack->OnCreated();
			uiStack->PushTop(ID_PANE_ISOMETRIC_GAME_VIEW);
			uiStack->PushTop(ID_PANE_GUI_WORLD_INTERFACE);
			levelLoader->Load(L"!levels/level1.sxy", false);	
		}

		virtual auto OnFrameUpdated(const IUltraClock& clock) -> uint32 // outputs ms sleep for next frame
		{
			e.postbox.Deliver();

			TimestepEvent timestep{ clock.Start(), clock.FrameStart(), clock.FrameDelta(), clock.Hz() };
			e.postbox.SendDirect(timestep);

			levelLoader->SyncWithModifiedFiles();

			e.renderer.Render(uiStack->Scene());
			return 5;
		}

		virtual void OnMouseEvent(const MouseEvent& me)
		{
			e.postbox.PostForLater(me);
		}

		virtual void OnKeyboardEvent(const KeyboardEvent& ke)
		{
			e.postbox.PostForLater(ke);
		}
	};
}

namespace Dystopia
{
	IApp* CreateDystopiaApp(IRenderer& renderer, IInstallation& installatiion)
	{
		return new DystopiaApp(renderer, installatiion);
	}
}