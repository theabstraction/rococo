#include "dystopia.h"
#include <rococo.renderer.h>
#include <rococo.io.h>
#include <vector>
#include "meshes.h"
#include "human.types.h"
#include "rococo.ui.h"
#include "dystopia.post.h"
#include "dystopia.ui.h"

using namespace Dystopia;
using namespace Rococo;
using namespace Rococo::Fonts;

namespace
{
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

	class DystopiaApp : public IApp, public IEventCallback<GuiEventArgs>, public IUIPaneFactory, public Post::IRecipient
	{
	private:
		AutoFree<Post::IPostboxSupervisor> postbox;
		AutoFree<IUIStackSupervisor> uiStack;
		AutoFree<IGuiSupervisor> gui;
		AutoFree<IDebuggerWindow> debuggerWindow;
		AutoFree<ISourceCache> sourceCache;
		AutoFree<IMeshLoader> meshes;
		AutoFree<IControlsSupervisor> controls;
		AutoFree<IBitmapCacheSupervisor> bitmaps;
		AutoFree<ILevelSupervisor> level;
		Environment e; // N.B some instances use e in constructor before e is initialized - this is to create a reference for internal use.
		HumanFactory humanFactory;	
		AutoFree<ILevelLoader> levelLoader;
		AutoFree<IUIControlPane> isometricGameWorldView;
		AutoFree<IUIPaneSupervisor> statsPaneSelf;
		AutoFree<IUIPaneSupervisor> inventoryPaneSelf;
	
	public:
		DystopiaApp(IRenderer& _renderer, IInstallation& _installation) :
			postbox(Post::CreatePostbox()),
			uiStack(CreateUIStack(*postbox)),
			gui(CreateGui(e, *uiStack)),
			debuggerWindow(CreateDebuggerWindow(&_renderer.Window())),
			sourceCache(CreateSourceCache(_installation)),
			meshes(CreateMeshLoader(_installation, _renderer, *sourceCache)),
			controls(CreateControlMapper(_installation, *sourceCache)),
			bitmaps(CreateBitmapCache(_installation, _renderer)),
			level(CreateLevel(e, humanFactory)),
			e{ _installation, _renderer, *debuggerWindow, *sourceCache, *meshes, *gui, *uiStack, *postbox, *controls, *bitmaps, *level },
			levelLoader(CreateLevelLoader(e)),
			isometricGameWorldView(CreatePaneIsometric(e)),
			statsPaneSelf(CreatePaneStats(e)),
			inventoryPaneSelf(CreateInventoryPane(e))
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
			case ID_PANE_STATS:
				return statsPaneSelf;
			case ID_PANE_INVENTORY_SELF:
				return inventoryPaneSelf;
			default:
				return nullptr;
			}
		}

		virtual void OnEvent(GuiEventArgs& args)
		{

		}

		void Examine(const VerbExamine& target)
		{
			auto* inv = level->GetInventory(target.entityId);
			if (inv)
			{
				auto* item = inv->GetItem(target.inventoryIndex);
				if (item)
				{
					AutoFree<IStringBuilder> sb(CreateSafeStringBuilder(4096));
					
					auto* rwd = item->GetRangedWeaponData();
					if (rwd)
					{
						if (rwd->muzzleVelocity < 300.0f)
						{
							sb->AppendFormat(L"Ranged Weapon:\n\t\t\tMuzzle Velocity: %.0f m/s", rwd->muzzleVelocity);
						}
						else if (rwd->muzzleVelocity < 3000)
						{
							sb->AppendFormat(L"Ranged Weapon:\n\t\t\tMuzzle Velocity: MACH %.2f", rwd->muzzleVelocity / 330.0f);
						}
						else if (rwd->muzzleVelocity < 100000000)
						{
							sb->AppendFormat(L"Ranged Weapon:\n\t\t\tMuzzle Velocity: %.2f km/s", rwd->muzzleVelocity / 1000.0f);
						}
						else
						{
							sb->AppendFormat(L"Ranged Weapon:\n\t\t\tParticle beam");
						}
					}

					uiStack->PushTop(CreateDialogBox(e, *this, L"Examine...", *sb, L"", { 640, 480 }, 100, 50), ID_PANE_GENERIC_DIALOG_BOX);
				}
			}
		}

		virtual void OnPost(Post::POST_TYPE id, const void* buffer, uint64 nBytes)
		{
			if (id == Post::POST_TYPE_EXAMINE)
			{
				auto& target = Post::InterpretAs<VerbExamine>(id, buffer, nBytes);
				Examine(target);
			}
		}

		virtual void Free()
		{ 
			delete this;
		}

		virtual void OnCreated()
		{
			uiStack->OnCreated();
			uiStack->PushTop(ID_PANE_ISOMETRIC_GAME_VIEW);
			uiStack->PushTop(ID_PANE_STATS);

			postbox->Subscribe(Post::POST_TYPE_EXAMINE, this);

			InitControlMap(*controls);
			controls->LoadMapping(L"!controls.cfg");
			levelLoader->Load(L"!levels/level1.sxy", false);	
		}

		virtual auto OnFrameUpdated(const IUltraClock& clock) -> uint32 // outputs ms sleep for next frame
		{
			levelLoader->SyncWithModifiedFiles();
			TimestepEvent timestep{ clock.Start(), clock.FrameStart(), clock.FrameDelta(), clock.Hz() };
			postbox->PostForLater(timestep, true);	
			postbox->Deliver();
			e.renderer.Render(uiStack->Scene());
			return 5;
		}

		virtual void OnMouseEvent(const MouseEvent& me)
		{
			postbox->PostForLater(me, true);
		}

		virtual void OnKeyboardEvent(const KeyboardEvent& ke)
		{
			postbox->PostForLater(ke, true);
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