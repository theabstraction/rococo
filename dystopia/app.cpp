#include "dystopia.h"
#include <rococo.renderer.h>
#include <rococo.io.h>
#include <rococo.sexy.ide.h>
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

	struct : IScene
	{
		virtual RGBA GetClearColour() const
		{
			return RGBA(0.5f, 0, 0);
		}

		virtual void RenderGui(IGuiRenderContext& grc)
		{
			GuiMetrics metrics;
			grc.Renderer().GetGuiMetrics(metrics);

			Graphics::RenderHorizontalCentredText(grc, L"Loading...", RGBAb(255, 255, 255, 255), 0, { metrics.cursorPosition.x >> 1, 100 });
		}

		virtual void RenderObjects(IRenderContext& rc)
		{

		}

      virtual void AddOverlay(int zorder, IUIOverlay* overlay)
      {

      }

      virtual void RemoveOverlay(IUIOverlay* overlay)
      {

      }
	} levelLoadScene;
}

namespace
{
   static cstr paneName = L"!gui/panes.sxy";

	class DystopiaApp : 
		public IDystopiaApp,
		public IEventCallback<GuiEventArgs>,
		public IUIPaneFactory, 
		public Post::IRecipient,
		public IHumanFactory,
		public IEventCallback<ContextMenuItem>,
      private Rococo::IEventCallback<ScriptCompileArgs>,
      private Rococo::IEventCallback<FileModifiedArgs>
	{
	private:
		AutoFree<Post::IPostboxSupervisor> postbox;
		AutoFree<IUIStackSupervisor> uiStack;
		AutoFree<IGuiSupervisor> gui;
		AutoFree<IDebuggerWindow> debuggerWindow;
      AutoFree<IDE::IScriptExceptionHandler> exceptionHandler;
		AutoFree<ISourceCache> sourceCache;
		AutoFree<IMeshLoader> meshes;
		AutoFree<IControlsSupervisor> controls;
		AutoFree<IBitmapCacheSupervisor> bitmaps;
		AutoFree<ILevelSupervisor> level;
		AutoFree<IBoneLibrarySupervisor> boneLibrary;
		AutoFree<IJournalSupervisor> journal;
		AutoFree<ILevelLoader> levelLoader;
      AutoFree<UI::IUIBuilderSupervisor> scriptableWidgets;
		Environment e; // N.B some instances use e in constructor before e is initialized - this is to create a reference for internal use.
		AutoFree<IUIControlPane> isometricGameWorldView;
		AutoFree<IUIPaneSupervisor> statsPaneSelf;
		AutoFree<IUIPaneSupervisor> inventoryPaneSelf;
      AutoFree<IUIPaneSupervisor> cvPaneSelf;
		AutoFree<IUIPaneSupervisor> personalInfoPanel;
		AutoFree<IUIPaneSupervisor> journalPane;
      
      Rococo::IDE::IPersistentScript* paneScript;
		
	public:
		DystopiaApp(IRenderer& _renderer, IInstallation& _installation) :
			postbox(Post::CreatePostbox()),
			uiStack(CreateUIStack(*postbox)),
			gui(CreateGui(e, *uiStack)),
			debuggerWindow(IDE::CreateDebuggerWindow(_renderer.Window())),
         exceptionHandler(UseDialogBoxForScriptException(debuggerWindow->GetDebuggerWindowControl())),
			sourceCache(CreateSourceCache(_installation)),
			meshes(CreateMeshLoader(_installation, _renderer, *sourceCache)),
			controls(CreateControlMapper(_installation, *sourceCache)),
			bitmaps(CreateBitmapCache(_installation, _renderer)),
			level(CreateLevel(e, *this)),
			boneLibrary(CreateBoneLibrary(_installation, _renderer, *sourceCache)),
			journal(CreateJournal(e)),
			levelLoader(CreateLevelLoader(e)),
         scriptableWidgets(UI::CreateScriptableUIBuilder()),
			// remember that order of construction here is order fields appear in the private section above, not in the order in this constructor
			e{ _installation, _renderer, *debuggerWindow, *exceptionHandler, *sourceCache, *meshes, *boneLibrary, *gui, *uiStack, *postbox, *controls, *bitmaps, *level, *journal, *levelLoader, *scriptableWidgets},
			isometricGameWorldView(CreatePaneIsometric(e)),
			statsPaneSelf(CreatePaneStats(e)),
			inventoryPaneSelf(CreateInventoryPane(e)),
         cvPaneSelf(CreateCVPane(e)),
			personalInfoPanel(CreatePersonalInfoPanel(e)),
			journalPane(CreateJournalPane(e)),
         paneScript(nullptr)
		{
			uiStack->SetFactory(*this);
			gui->SetEventHandler(this);
			level->OnCreated();
			journal->PostConstruct();

         GuiMetrics metrics;
         _renderer.GetGuiMetrics(metrics);
         scriptableWidgets->Resize(metrics.screenSpan);
		}
		
		~DystopiaApp()
		{
         if (paneScript)
         {
            paneScript->Free();
            paneScript = nullptr;
         }
		}

      void OnEvent(ScriptCompileArgs& args)
      {
         UI::AddNativeCalls_DystopiaUIIUIBuilder(args.ss, &WidgetBuilder());
      }

      void OnEvent(FileModifiedArgs& args)
      {
         meshes->UpdateMesh(args.resourceName);
         boneLibrary->UpdateLib(args.resourceName);
         
         if (DoesModifiedFilenameMatchResourceName(args.resourceName, paneName))
         {
            e.sourceCache.Release(paneName);
            LoadPanes();
         }
      }

      void LoadPanes()
      {
         if (paneScript)
         {
            paneScript->Free();
            paneScript = nullptr;
         }

         try
         {
            paneScript = IDE::CreatePersistentScript(16384, e.sourceCache, e.debuggerWindow, paneName, (int32)1_megabytes, *this, e.exceptionHandler);

            struct : IArgEnumerator
            {
               virtual void PushArgs(IArgStack& args)
               {
               }

               virtual void PopOutputs(IOutputStack& args)
               {
               }
            } args;
            paneScript->ExecuteFunction(L"Main", args, e.exceptionHandler);
         }
         catch (IException&)
         {
            
         }
      }

      UI::IUIBuilder& WidgetBuilder()
      {
         return scriptableWidgets->Builder();
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
         case ID_PANE_CV:
            return cvPaneSelf;
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
			if (args.controlScript[0] == L'0' && args.controlScript[1] == 0)
			{
				return;
			}

			struct : IArgEnumerator
			{
				virtual void PushArgs(IArgStack& args)
				{

				}

				virtual void PopOutputs(IOutputStack& output)
				{

				}
			} nullArgs;
			e.levelLoader.ExecuteLevelFunction(args.controlScript, nullArgs);
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

		virtual void OnCreate()
		{
			uiStack->OnCreated();
			uiStack->PushTop(ID_PANE_ISOMETRIC_GAME_VIEW);
			uiStack->PushTop(ID_PANE_STATS);

			postbox->Subscribe<VerbExamine>(this);
			postbox->Subscribe<SelectItemOnGround>(this);

			InitControlMap(*controls);
			controls->LoadMapping(L"!controls.cfg");

			boneLibrary->Reload(L"!bone.library.sxy");

         LoadPanes();

			levelLoader->Load(L"!levels/level1.sxy", false);	
		}

		virtual auto OnFrameUpdated(const IUltraClock& clock) -> uint32 // outputs ms sleep for next frame
		{
         GetOS(e).EnumerateModifiedFiles(*this);
			
			TimestepEvent timestep{ clock.Start(), clock.FrameStart(), clock.FrameDelta(), clock.Hz() };
			postbox->PostForLater(timestep, true);	
			postbox->Deliver();
			e.journal.UpdateGoals();	

			if (e.levelLoader.NeedsUpdate())
			{
				e.renderer.Render(levelLoadScene);
				// Allow journal and postbox to settle down sending messages to each other, without updating timestep
				for (int i = 0; i < 10; ++i)
				{
					e.journal.UpdateGoals();
					postbox->Deliver();
				}
				e.levelLoader.Update(); // Load the level just after post is delivered so that outstanding post does not hit the new level
			}
			
         GuiMetrics metrics;
         e.renderer.GetGuiMetrics(metrics);
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

      virtual cstr Title() const
      {
         return L"Dystopia";
      }
	};
}

namespace Dystopia
{
   IDystopiaApp* CreateDystopiaApp(IRenderer& renderer, IInstallation& installatiion)
	{
		return new DystopiaApp(renderer, installatiion);
	}
}