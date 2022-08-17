#include "hv.h"
#include "hv.events.h"
#include <rococo.strings.h>
#include <rococo.maths.h>
#include <rococo.widgets.h>
#include <rococo.mplat.h>
#include <rococo.textures.h>
#include <rococo.ui.h>
#include <rococo.io.h>

#include <vector>
#include <algorithm>
#include <unordered_map>

#include <rococo.rings.inl> 

namespace HV
{
	namespace Events
	{
		const EventIdRef evChangeDefaultTextureId = "editor.textureId.change"_event;
		const EventIdRef evPopulateTabs = "tabs.populate"_event;
	}

	void SaveLevelAsFunction(ISectors& sectors, StringBuilder& sb);
}

namespace
{
	using namespace HV;
	using namespace Rococo;
	using namespace Rococo::Widgets;
	using namespace Rococo::Entities;
	using namespace Rococo::IO;

	auto evEditorToolsVScrollSet = "editor.tools.vscroll_set"_event;

	EventIdRef evScrollChanged = "editor.tools.vscroll_ui"_event;
	EventIdRef evScrollSet = "editor.tools.vscroll_set"_event;
	EventIdRef evScrollGet = "editor.tools.vscroll_get"_event;
	EventIdRef evScrollSendKey = "editor.tools.vscroll_sendkey"_event;
	EventIdRef evScrollSendMouse = "editor.tools.vscroll_sendmouse"_event;

	class ToggleEventHandler;
	struct ToggleStateChanged
	{
		ToggleEventHandler* handler;
	};

	class ToggleEventHandler : public IObserver
	{
		std::vector<cstr> names;
		IPublisher& publisher;
		EventIdRef id;
		int state = 0;
		IEventCallback<ToggleStateChanged>* eventHandler = nullptr;

	public:
		ToggleEventHandler(cstr handlerName, IPublisher& _publisher, std::vector<cstr> _names) :
			id(_publisher.CreateEventIdFromVolatileString(handlerName)), publisher(_publisher), names(_names)
		{
			publisher.Subscribe(this, id);
		}

		~ToggleEventHandler()
		{
			publisher.Unsubscribe(this);
		}

		void AddHandler(IEventCallback<ToggleStateChanged>* _eventHandler)
		{
			eventHandler = _eventHandler;
		}

		int State() const
		{
			return state;
		}

		void SetState(int index)
		{
			if (index < 0 || index >= (int32)names.size())
			{
				Throw(0, "ToggleEventHandler::SetState. Index out of bounds: %d. [0,%d]", index, names.size());
			}

			this->state = index;
		}

		void OnEvent(Event& ev) override
		{
			if (ev == id)
			{
				auto& toe = As<TextOutputEvent>(ev);
				if (toe.isGetting)
				{
					SafeFormat(toe.text, sizeof(toe.text), "%s", names[state]);
				}
				else
				{
					for (int32 i = 0; i < names.size(); ++i)
					{
						if (Eq(names[i], toe.text))
						{
							this->state = i;

							if (eventHandler)
							{
								eventHandler->OnEvent(ToggleStateChanged{ this });
							}
							break;
						}
					}
				}
			}
		}
	};

	class Editor :
		public IEditor,
		public IUIElement,
		private IObserver, 
		IEventCallback<ToggleStateChanged>,
		public IEditorState,
		public IEventCallback<BloodyNotifyArgs>
	{
		AutoFree<IWorldMapSupervisor> map;
		AutoFree<ISectorBuilderEditor> editMode_SectorBuilder;
		AutoFree<ISectorEditor> editMode_SectorEditor;
		GuiMetrics metrics;
		AutoFree<IStatusBar> statusbar;
		Platform& platform;
		IFPSGameMode& fpsGameMode;
		IPlayerSupervisor& players;
		AutoFree<ITextureList> textureList;
		bool initialized = false;

		ToggleEventHandler editModeHandler;
		ToggleEventHandler textureTargetHandler;
		ToggleEventHandler scrollLock;
		ToggleEventHandler transparency;

		AutoFree<IBloodyPropertySetEditorSupervisor> objectLayoutEditor;
		AutoFree<IBloodyPropertySetEditorSupervisor> wallEditor;
		AutoFree<IBloodyPropertySetEditorSupervisor> floorEditor;
		AutoFree<IBloodyPropertySetEditorSupervisor> ceilingEditor;
		AutoFree<IBloodyPropertySetEditorSupervisor> corridorEditor;
		AutoFree<IBloodyPropertySetEditorSupervisor> lightEditor;

		AutoFree<IBloodyPropertySetEditorSupervisor> ambienceEditor;

		AutoFree<IPaneBuilderSupervisor> fileBrowserPane;

		char levelpath[IO::MAX_PATHLEN] = { 0 };

		cstr LevelFilename() const
		{
			return levelpath;
		}

		bool IsScrollLocked() const override
		{
			return scrollLock.State() == 1;
		}

		bool OnKeyboardEvent(const KeyboardEvent& k) override
		{
			Key key = platform.keyboard.GetKeyFromEvent(k);
			auto* action = platform.keyboard.GetAction(key.KeyName);

			if (action && !key.isPressed && Eq(action, "gui.editor.toggle"))
			{
				TEventArgs<bool> disable;
				disable.value = false;
				platform.publisher.Publish(disable, HV::Events::evEnableEditor);
				return true;
			}
			else
			{
				return EditMode().OnKeyboardEvent(k);
			}
		}

		void OnRawMouseEvent(const MouseEvent& key) override
		{
		}

		void OnMouseMove(Vec2i cursorPos, Vec2i delta, int dWheel)  override
		{
			EditMode().OnMouseMove(cursorPos, delta, dWheel);
		}

		void OnMouseLClick(Vec2i cursorPos, bool clickedDown)  override
		{
			EditMode().OnMouseLClick(cursorPos, clickedDown);
		}

		void OnMouseRClick(Vec2i cursorPos, bool clickedDown)  override
		{
			EditMode().OnMouseRClick(cursorPos, clickedDown);
		}

		IEditMode& EditMode()
		{
			return editModeHandler.State() == 0 ? editMode_SectorBuilder->Mode() : editMode_SectorEditor->Mode();
		}

		void OnEvent(ToggleStateChanged& ev)
		{
			if (ev.handler == &textureTargetHandler)
			{
				int state = textureTargetHandler.State();
				cstr textureName = editMode_SectorBuilder->GetTexture(state);
				textureList->ScrollTo(textureName);
			}
		}

		void OnEvent(Event& ev) override
		{
			if (ev == HV::Events::evChangeDefaultTextureId)
			{
				auto& cdt = As<HV::Events::ChangeDefaultTextureEvent>(ev);

				int32 textureTargetIndex = textureTargetHandler.State();
				editMode_SectorBuilder->SetTexture(textureTargetIndex, cdt.wallName);
			}
		}

		IPropertyTarget* target = nullptr;

		virtual void SetPropertyTargetToSuccessor()
		{
			if (this->target != nullptr)
			{
				this->target->Assign(nullptr);
			}

			this->target = nullptr;
		}

		void SetPropertyTarget(IPropertyTarget* target) override
		{
			if (this->target != nullptr)
			{
				this->target->Assign(nullptr);
			}

			wallEditor->Clear();
			floorEditor->Clear();
			ceilingEditor->Clear();
			corridorEditor->Clear();
			lightEditor->Clear();
			ambienceEditor->Clear();

			if (target == nullptr)
			{
				target = fpsGameMode.GetPropertyTarget();

				PopulateTabsEvent tbe;
				tbe.populatorName = "editor.tabs";

				PopulateTabsEvent::TabRef globalTabs[] =
				{
					{ "Ambience",   "editor.tab.ambience", 80 }
				};

				tbe.numberOfTabs = 1;
				tbe.tabArray = globalTabs;

				platform.publisher.Publish(tbe, HV::Events::evPopulateTabs);

				target->GetProperties("Ambient", *ambienceEditor);
			}
			
			this->target = target;

			if (target)
			{		
				target->Assign(this);
			}
		}

		void BindSectorPropertiesToPropertyEditor(IPropertyTarget* target) override
		{
			PopulateTabsEvent tbe;
			tbe.populatorName = "editor.tabs";

			PopulateTabsEvent::TabRef sectorTabs[] =
			{
				{ "Objects",	"editor.tab.objects", 65 },
				{ "Walls",		"editor.tab.walls",   50 },
				{ "Floor",		"editor.tab.floor",   50 },
				{ "Ceiling",	"editor.tab.ceiling", 60 },
				{ "Corridor",   "editor.tab.corridor",75 },
				{ "Lights",		"editor.tab.lights",  55 },
				{ "Tags",		"editor.tab.tags",    55 }
			};

			tbe.numberOfTabs = 6;
			tbe.tabArray = sectorTabs;

			platform.publisher.Publish(tbe, HV::Events::evPopulateTabs);

			target->GetProperties("walls", *wallEditor);
			target->GetProperties("floor", *floorEditor);
			target->GetProperties("ceiling", *ceilingEditor);
			target->GetProperties("corridor", *corridorEditor);
			target->GetProperties("lights", *lightEditor);
			target->GetProperties("tags", *lightEditor);
		}

		void Render(IGuiRenderContext& grc, const GuiRect& absRect) override
		{
			if (!initialized)
			{
				SetPropertyTarget(nullptr);
				initialized = true;
			}

			grc.Renderer().GetGuiMetrics(metrics);
			map->Render(grc, EditMode().GetHilight(), transparency.State() == 1);

			EditMode().Render(grc, absRect);

			map->RenderTopGui(grc, players.GetPlayer(0)->GetPlayerEntity());

			GuiRect statusRect{ absRect.left, absRect.bottom - 24, absRect.right, absRect.bottom };
			statusbar->Render(grc, statusRect);
		}

		void Free() override
		{
			delete this;
		}

		void OnEditorNew(cstr command)
		{
			map->Sectors().Builder()->Clear();
			editMode_SectorEditor->CancelHilight();
		}

		void OnEditorLoad(cstr command)
		{
			struct BrowserRules_LoadLevel : public IBrowserRules
			{
				IPublisher& publisher;
				Editor& editor;
				HString lastError;

				BrowserRules_LoadLevel(IPublisher& _publisher, Editor& _editor) :
					publisher(_publisher), editor(_editor)
				{

				}

				void GetInitialFilename(U32FilePath& path) const override
				{
					PathFromAscii(editor.LevelFilename(), '/', path);
				}

				void GetRoot(U32FilePath& path) const override
				{
					path = { U"!scripts/hv/levels/" };
				}

				cstr GetLastError() const override
				{
					return lastError;
				}

				void GetCaption(char* caption, size_t capacity) override
				{
					SafeFormat(caption, capacity, "Select level file to load...");
				}

				void Free() override
				{
					delete this;
				}

				bool Select(const U32FilePath& levelFilename) override
				{
					U8FilePath pingPath;
					ToU8(levelFilename, pingPath);

					if (EndsWith(pingPath, ".level.sxy"))
					{
						editor.OnLoadLevelFileNameSelected(pingPath);
						return true;
					}
					else
					{
						lastError = "Filename must have extension '.level.sxy'";
						OS::BeepWarning();
						return false;
					}
				}
			};

			struct BrowseRulesLoadLevelFactory : public IBrowserRulesFactory
			{
				Editor* editor;
				IPublisher* publisher;
				IBrowserRules* CreateRules() override
				{
					return new BrowserRules_LoadLevel(*publisher, *editor);
				}

				cstr GetPanePingPath() const
				{
					return "!scripts/panel.open.sxy";
				}
			} forLevelFilename;
			forLevelFilename.editor = this;
			forLevelFilename.publisher = &platform.publisher;
			platform.utilities.BrowseFiles(forLevelFilename);
		}

		void OnLoadLevelFileNameSelected(cstr pingPath)
		{
			try
			{
				Load(pingPath);

				auto root = "!scripts/hv/levels/"_fstring;

				char shortPingPath[IO::MAX_PATHLEN];

				if (StartsWith(pingPath, root))
				{
					SafeFormat(shortPingPath, IO::MAX_PATHLEN, "%s", pingPath + root.length);
				}
				else
				{
					SafeFormat(shortPingPath, IO::MAX_PATHLEN, "%s", pingPath);
				}

				platform.utilities.AddSubtitle(shortPingPath);
			}
			catch (IException& ex)
			{
				char sysMessage[1024];
				Rococo::OS::FormatErrorMessage(sysMessage, 1024, ex.ErrorCode());

				char errorBuffer[1024];
				SafeFormat(errorBuffer, 1024, "Error loading level. %s %s", sysMessage, ex.Message());

				platform.messaging.Log(to_fstring(errorBuffer));

				platform.utilities.ShowErrorBox(platform.mainWindow, ex, "Error loading level file");
			}

			platform.utilities.ShowBusy(false, "", "");
		}

		void OnEditorSave(cstr command)
		{
			struct BrowserRules_SaveLevel : public IBrowserRules
			{
				IPublisher& publisher;
				Editor& editor;
				HString lastError;

				BrowserRules_SaveLevel(IPublisher& _publisher, Editor& _editor) :
					publisher(_publisher), editor(_editor)
				{

				}

				void GetInitialFilename(U32FilePath& path) const override
				{
					PathFromAscii(editor.LevelFilename(), '/', path);
				}

				void GetRoot(U32FilePath& path) const
				{
					path = { U"!scripts/hv/levels/" };
				}

				cstr GetLastError() const
				{
					return lastError;
				}

				void GetCaption(char* caption, size_t capacity) override
				{
					SafeFormat(caption, capacity, "Select file to save level as...");
				}

				void Free() override
				{
					delete this;
				}

				bool Select(const U32FilePath& levelFilename) override
				{
					U8FilePath pingPath;
					ToU8(levelFilename, pingPath);

					if (EndsWith(pingPath, ".level.sxy"))
					{
						editor.OnSaveLevelFileNameSelected(pingPath);
						return true;
					}
					else
					{
						lastError = "Filename must have extension '.level.sxy'";
						OS::BeepWarning();
						return false;
					}
				}
			};

			struct BrowseRulesLoadLevelFactory : public IBrowserRulesFactory
			{
				Editor* editor;
				IPublisher* publisher;
				IBrowserRules* CreateRules() override
				{
					return new BrowserRules_SaveLevel(*publisher, *editor);
				}

				cstr GetPanePingPath() const
				{
					return "!scripts/panel.saveas.sxy";
				}
			} forLevelFilename;
			forLevelFilename.editor = this;
			forLevelFilename.publisher = &platform.publisher;
			platform.utilities.BrowseFiles(forLevelFilename);
		}

		void OnSaveLevelFileNameSelected(cstr pingPath)
		{
			WideFilePath sysPath;
			platform.installation.ConvertPingPathToSysPath(pingPath, sysPath);

			Save(sysPath);
			SafeFormat(levelpath, Rococo::IO::MAX_PATHLEN, "%s", pingPath);

			char shortPingName[256];
			SafeFormat(shortPingName, 256, "%ls", sysPath.buf);
			platform.utilities.AddSubtitle(shortPingName);
		}

		void Load(cstr pingName)
		{
			HV::Events::SetNextLevelEvent setNextLevelEvent;
			setNextLevelEvent.name = pingName;

			platform.publisher.Publish(setNextLevelEvent, HV::Events::evSetNextLevel);
		}

		void Save(const wchar_t* filename)
		{
			std::vector<char> buffer;
			buffer.resize(1024_kilobytes);

			char* buffer0 = &buffer[0];
			memset(buffer0, 0, buffer.size());

			StackStringBuilder sb(buffer0, buffer.size());

			sb.AppendFormat("(' #file.type hv.level)\n\n");

			sb.AppendFormat("(' #include\n\t\"!scripts/mplat_sxh.sxy\""
				"\n\t\"!scripts/hv_sxh.sxy\""
				"\n\t\"!scripts/types.sxy\""
				"\n\t\"!scripts/hv/hv.types.sxy\""
				"\n\t\"!scripts/mplat_types.sxy\""
				")\n\n");

			sb.AppendFormat("(namespace EntryPoint)\n\t(alias Main EntryPoint.Main)\n\n");
			sb.AppendFormat("(function Main(Int32 id)->(Int32 exitCode) :\n");
			sb.AppendFormat("\t(AddSectorsToLevel)\n");
			sb.AppendFormat(")\n\n");

			HV::SaveLevelAsFunction(map->Sectors(), sb);

			try
			{
				platform.utilities.SaveBinary(filename, buffer0, sb.Length());
			}
			catch (IException& ex)
			{
				platform.utilities.ShowErrorBox(platform.mainWindow, ex, ex.Message());
			};
		}

		void OnEvent(BloodyNotifyArgs& args)
		{
			if (target != nullptr)
			{
				target->NotifyChanged(args);
			}

			map->Sectors().NotifyChanged();
		}

	public:
		Editor(Platform& _platform, IPlayerSupervisor& _players, ISectors& sectors, IFPSGameMode& _fpsGameMode) :
			platform(_platform),
			fpsGameMode(_fpsGameMode),
			players(_players),
			map(CreateWorldMap(_platform, sectors)),
			textureList(CreateTextureList(_platform)),
			editMode_SectorBuilder(CreateSectorBuilder(_platform.publisher, *map)),
			editMode_SectorEditor(CreateSectorEditor(_platform, *map, _platform.mainWindow)),
			statusbar(CreateStatusBar(_platform.publisher)),
			editModeHandler("editor.edit_mode", _platform.publisher, { "v", "s" }),
			textureTargetHandler("editor.texture.target", _platform.publisher, { "w", "f", "c" }),
			scrollLock("editor.texture.lock", _platform.publisher, { "U", "L" }),
			transparency("editor.transparency", _platform.publisher, {"o", "t"})
		{
			REGISTER_UI_EVENT_HANDLER(platform.gui, this, Editor, OnEditorNew, "editor.new", nullptr);
			REGISTER_UI_EVENT_HANDLER(platform.gui, this, Editor, OnEditorLoad, "editor.load", nullptr);
			REGISTER_UI_EVENT_HANDLER(platform.gui, this, Editor, OnEditorSave, "editor.save", nullptr);

			objectLayoutEditor = platform.utilities.CreateBloodyPropertySetEditor(*this);
			wallEditor = platform.utilities.CreateBloodyPropertySetEditor(*this);
			floorEditor = platform.utilities.CreateBloodyPropertySetEditor(*this);
			ceilingEditor = platform.utilities.CreateBloodyPropertySetEditor(*this);
			corridorEditor = platform.utilities.CreateBloodyPropertySetEditor(*this);
			lightEditor = platform.utilities.CreateBloodyPropertySetEditor( *this);
			ambienceEditor = platform.utilities.CreateBloodyPropertySetEditor(*this);

			platform.publisher.Subscribe(this, HV::Events::evChangeDefaultTextureId);

			transparency.SetState(0);
			editModeHandler.SetState(0);
			textureTargetHandler.SetState(0);
			scrollLock.SetState(0);

			textureTargetHandler.AddHandler(this);

			platform.gui.RegisterPopulator("sector_editor", this);

			editMode_SectorEditor->SetEditor(this);

			platform.gui.RegisterPopulator("editor.tab.objects", &(*objectLayoutEditor));
			platform.gui.RegisterPopulator("editor.tab.walls", &(*wallEditor));
			platform.gui.RegisterPopulator("editor.tab.floor", &(*floorEditor));
			platform.gui.RegisterPopulator("editor.tab.ceiling", &(*ceilingEditor));
			platform.gui.RegisterPopulator("editor.tab.corridor", &(*corridorEditor));
			platform.gui.RegisterPopulator("editor.tab.lights", &(*lightEditor));
			platform.gui.RegisterPopulator("editor.tab.ambience", &(*ambienceEditor));

			sectors.BindProperties(*objectLayoutEditor);
		}

		~Editor()
		{
			platform.gui.UnregisterPopulator(&(*objectLayoutEditor));
			platform.gui.UnregisterPopulator(&(*wallEditor));
			platform.gui.UnregisterPopulator(&(*floorEditor));
			platform.gui.UnregisterPopulator(&(*ceilingEditor));
			platform.gui.UnregisterPopulator(&(*corridorEditor));
			platform.gui.UnregisterPopulator(&(*lightEditor));
			platform.gui.UnregisterPopulator(&(*ambienceEditor));
			platform.gui.UnregisterPopulator(this);
			platform.publisher.Unsubscribe(this);

			if (target) target->Assign(nullptr);
			target = nullptr;
		}

		virtual cstr TextureName(int index) const
		{
			return editMode_SectorBuilder->GetTexture(index);
		}
	};
}

namespace HV
{
	IEditor* CreateEditor(Platform& platform, IPlayerSupervisor& players, ISectors& sectors, IFPSGameMode& fpsGameMode)
	{
		return new Editor(platform, players, sectors, fpsGameMode);
	}
}