#include "dystopia.h"
#include <rococo.renderer.h>
#include <rococo.io.h>
#include <vector>
#include "meshes.h"
#include "human.types.h"
#include "rococo.ui.h"
#include "dystopia.post.h"
#include "dystopia.ui.h"
#include "dystopia.constants.h"
#include "skeleton.h"

using namespace Dystopia;
using namespace Rococo;
using namespace Rococo::Fonts;

namespace Dystopia
{
	namespace Verb
	{
		void Examine(const VerbExamine& target, Environment& e);
		void OpenItem(Environment& e, ID_ENTITY itemId, ID_ENTITY collectorId, Metres maxPickupRange);
		void PickupItem(Environment& e, ID_ENTITY itemId, ID_ENTITY collectorId, Metres maxPickupRange);
		void ShowSelectOptions(ID_ENTITY id, Environment& e, IEventCallback<ContextMenuItem>& handler);
	}
}

namespace
{
	class DystopiaApp : 
		public IApp,
		public IEventCallback<GuiEventArgs>,
		public IUIPaneFactory, 
		public Post::IRecipient,
		public IHumanFactory,
		public IEventCallback<ContextMenuItem>
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
		AutoFree<IBoneLibrarySupervisor> boneLibrary;
		AutoFree<IJournalSupervisor> journal;
		AutoFree<ILevelLoader> levelLoader;
		Environment e; // N.B some instances use e in constructor before e is initialized - this is to create a reference for internal use.
		AutoFree<IUIControlPane> isometricGameWorldView;
		AutoFree<IUIPaneSupervisor> statsPaneSelf;
		AutoFree<IUIPaneSupervisor> inventoryPaneSelf;
		AutoFree<IUIPaneSupervisor> personalInfoPanel;
		AutoFree<IUIPaneSupervisor> journalPane;
		
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
			level(CreateLevel(e, *this)),
			boneLibrary(CreateBoneLibrary(_installation, _renderer, *sourceCache)),
			journal(CreateJournal(e)),
			levelLoader(CreateLevelLoader(e)),
			// remember that order of construction here is order fields appear in the private section above, not in the order in this constructor
			e{ _installation, _renderer, *debuggerWindow, *sourceCache, *meshes, *boneLibrary, *gui, *uiStack, *postbox, *controls, *bitmaps, *level, *journal, *levelLoader },
			isometricGameWorldView(CreatePaneIsometric(e)),
			statsPaneSelf(CreatePaneStats(e)),
			inventoryPaneSelf(CreateInventoryPane(e)),
			personalInfoPanel(CreatePersonalInfoPanel(e)),
			journalPane(CreateJournalPane(e))
		{
			uiStack->SetFactory(*this);
			gui->SetEventHandler(this);
			level->OnCreated();
			journal->PostConstruct();
		}
		
		~DystopiaApp()
		{
		}

		virtual IHumanAISupervisor* CreateHuman(ID_ENTITY id, HumanType typeId)
		{
			switch (typeId)
			{
			case HumanType_Bobby:
				return CreateBobby(id, e);
			case HumanType_Vigilante:
				return CreateVigilante(id, *isometricGameWorldView->PlayerIntent(), e);
			}
			return nullptr;
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
			case ID_PANE_PERSONAL_INFO:
				return personalInfoPanel;
			case ID_PANE_JOURNAL:
				return journalPane;
			default:
				return nullptr;
			}
		}

		virtual void OnEvent(GuiEventArgs& args)
		{

		}

		virtual void OnEvent(ContextMenuItem& item)
		{
			ID_ENTITY id((size_t)item.context);

			switch (item.commandId)
			{
			case ID_CONTEXT_COMMAND_EXAMINE:
				Verb::Examine(VerbExamine{ id, 0 }, e);
				break;
			case ID_CONTEXT_COMMAND_PICKUP:
				Verb::PickupItem(e, id, level->GetPlayerId(), PickupRange());
				break;
			case ID_CONTEXT_COMMAND_OPEN:
				Verb::OpenItem(e, id, level->GetPlayerId(), PickupRange());
			default:
				break;
			}
		}

		virtual void OnPost(const Mail& mail)
		{
			auto* target = Post::InterpretAs<VerbExamine>(mail);
			if (target)	Verb::Examine(*target, e);

			auto* selectItem = Post::InterpretAs<SelectItemOnGround>(mail);
			if (selectItem)
			{
				Verb::ShowSelectOptions(selectItem->id, e, *this);
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

			postbox->Subscribe<VerbExamine>(this);
			postbox->Subscribe<SelectItemOnGround>(this);

			InitControlMap(*controls);
			controls->LoadMapping(L"!controls.cfg");

			boneLibrary->Reload(L"!bone.library.sxy");

			levelLoader->Load(L"!levels/level1.sxy", false);	
		}

		virtual auto OnFrameUpdated(const IUltraClock& clock) -> uint32 // outputs ms sleep for next frame
		{
			levelLoader->SyncWithModifiedFiles();
			TimestepEvent timestep{ clock.Start(), clock.FrameStart(), clock.FrameDelta(), clock.Hz() };
			postbox->PostForLater(timestep, true);	
			postbox->Deliver();
			e.journal.UpdateGoals();
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