#include "hv.h"
#include <rococo.mplat.h>
#include <rococo.strings.h>
#include <rococo.rings.inl>
#include <vector>

namespace
{
	using namespace HV;

	static const EventIdRef evEditSectorLogic = "edit.sector.logic"_event;
	static const EventIdRef evPopulateTriggers = "editor.logic.enum_triggers"_event;

	struct LogicEditor: public IObserver
	{
		struct TriggerList: IEnumVector
		{
			ISector* sector = nullptr;
			int32 activeIndex = 0;

			int32 GetActiveIndex() const override
			{
				return activeIndex;
			}

			void SetActiveIndex(int32 index) override
			{
				activeIndex = index;
			}

			int32 operator[] (int32 index) const override
			{
				return (int32) sector->TriggersAndActions()[index].Type();
			}

			void SetValue(int32 index, int32 value) override
			{
				if (index < 0 || index >= (int32)sector->TriggersAndActions().TriggerCount())
				{
					Throw(0, "LogicEditor.TriggerList.SetValue(%d) index out of range)", index);
				}

				sector->TriggersAndActions()[index].SetType((TRIGGER_TYPE)value);
			}

			int32 Count() const  override
			{
				return sector->TriggersAndActions().TriggerCount();
			}
		} triggerList;
		Platform& platform;
		ISectors& sectors;
		ISector* sector = nullptr;

		AutoFree<IPaneBuilderSupervisor> logicPanel;

		LogicEditor(Platform& _platform, ISectors& _sectors) : 
			platform(_platform), sectors(_sectors)
		{
			logicPanel = platform.gui.BindPanelToScript("!scripts/hv/panel.logic.sxy");
			platform.publisher.Subscribe(this, evEditSectorLogic);
			platform.publisher.Subscribe(this, evUIInvoke);
			platform.publisher.Subscribe(this, evPopulateTriggers);
		}

		void OpenEditor()
		{
			auto id = sectors.GetSelectedSectorId();
			if (id != (size_t)-1)
			{
				sector = sectors.begin()[id];
				platform.gui.PushTop(logicPanel->Supervisor(), true);
			}
			else
			{
				sector = nullptr;		
			}
			
			triggerList.sector = sector;
			triggerList.activeIndex = 0;
		}

		void CloseEditor()
		{
			if (platform.gui.Top() == logicPanel->Supervisor())
			{
				platform.gui.Pop();
				sector = nullptr;
				triggerList.sector = nullptr;
			}
			else
			{
				Throw(0, "LogicEditor::CloseEditor -> the editor panel was not on top of the gui stack");
			}
		}

		void AddTrigger()
		{
			sector->TriggersAndActions().AddTrigger(triggerList.activeIndex);
		}

		void RemoveTrigger()
		{
			sector->TriggersAndActions().RemoveTrigger(triggerList.activeIndex);
		}

		void AddAction()
		{
			sector->TriggersAndActions().AddAction(triggerList.activeIndex);
		}

		void RemoveAction()
		{
			sector->TriggersAndActions().RemoveAction(triggerList.activeIndex);
		}

		void OnEvent(Event& ev) override
		{
			if (ev == evEditSectorLogic)
			{
				OpenEditor();
			}
			else if (ev == Rococo::Events::evUIInvoke)
			{
				auto& cmd = As<UIInvoke>(ev);
				if (Eq(cmd.command, "editor.logic.close"))
				{
					CloseEditor();
				}
				else if (Eq(cmd.command, "editor.logic.add_trigger"))
				{
					AddTrigger();
				}
				else if (Eq(cmd.command, "editor.logic.remove_trigger"))
				{
					RemoveTrigger();
				}
				else if (Eq(cmd.command, "editor.logic.add_action"))
				{
					AddAction();
				}
				else if (Eq(cmd.command, "editor.logic.remove_action"))
				{
					RemoveAction();
				}
			}
			else if (ev == evPopulateTriggers)
			{
				auto& pop = As<TEventArgs<Rococo::IEnumVector*>>(ev);
				if (triggerList.sector != nullptr)
				{
					pop.data = &triggerList;
				}
			}
		}
	};

	class SectorEditor : public ISectorEditor, private IEditMode
	{
		IWorldMap& map;
		GuiMetrics metrics = { 0 };
		Platform& platform;
		IEditorState* editor;
		Windows::IWindow& parent;
		LogicEditor logicEditor;

		void Render(IGuiRenderContext& grc, const GuiRect& rect) override
		{

		}

		bool OnKeyboardEvent(const KeyboardEvent& k) override
		{
			Key key = platform.keyboard.GetKeyFromEvent(k);

			auto* action = platform.keyboard.GetAction(key.KeyName);
			if (action && Eq(action, "gui.editor.sector.delete"))
			{
				if (!key.isPressed)
				{
					auto& s = map.Sectors();

					size_t litIndex = map.Sectors().GetSelectedSectorId();

					size_t nSectors = s.end() - s.begin();
					if (litIndex < nSectors)
					{
						s.Delete(s.begin()[litIndex]);
						map.Sectors().SelectSector(-1);
					}
				}
				return true;
			}

			return false;
		}

		void OnRawMouseEvent(const MouseEvent& key) override
		{
		}

		void OnMouseMove(Vec2i cursorPos, Vec2i delta, int dWheel) override
		{
			if (dWheel < 0)
			{
				map.ZoomIn(-dWheel);
			}
			else if (dWheel > 0)
			{
				map.ZoomOut(dWheel);
			}
		}

		void OnMouseLClick(Vec2i cursorPos, bool clickedDown) override
		{
			if (clickedDown)
			{
				map.GrabAtCursor();
			}
			else
			{
				map.ReleaseGrab();
			}
		}

		void PopupSectorContext(Vec2i cursorPos, int sectorIndex)
		{
			auto& menu = platform.utilities.PopupContextMenu();
			menu.Clear(0);
			menu.AddString(0, "Logic..."_fstring, "edit.sector.logic"_fstring, ""_fstring);
			menu.SetPopupPoint(cursorPos + Vec2i{ 25, 25 });
		}

		void OnMouseRClick(Vec2i cursorPos, bool clickedDown) override
		{
			if (!clickedDown)
			{
				Vec2 wp = map.GetWorldPosition(cursorPos);
				for (auto* s : map.Sectors())
				{
					int32 index = s->GetFloorTriangleIndexContainingPoint(wp);
					if (index >= 0)
					{
						auto& secs = map.Sectors();

						size_t nSectors = secs.end() - secs.begin();
						for (size_t i = 0; i < nSectors; ++i)
						{
							if (secs.begin()[i] == s)
							{
								if (map.Sectors().GetSelectedSectorId() == i)
								{
									// Already selected
									PopupSectorContext(cursorPos, (int32) i);
								}
								else
								{
									map.Sectors().SelectSector(i);
								}
								editor->SetPropertyTarget(secs.begin()[i]);
								editor->BindSectorPropertiesToPropertyEditor(secs.begin()[i]);
								return;
							}
						}
					}
				}

				map.Sectors().SelectSector(-1);
				editor->SetPropertyTarget(nullptr);
			}
		}
	public:
		SectorEditor(Platform& _platform, IWorldMap& _map, Windows::IWindow& _parent) :
			platform(_platform),
			map(_map),
			parent(_parent),
			editor(nullptr),
			logicEditor(_platform, _map.Sectors())
		{ 
		}

		~SectorEditor()
		{
		}

		IEditMode& Mode() override { return *this; }
		const ISector* GetHilight() const override
		{
			auto& secs = map.Sectors();
			size_t nSectors = secs.end() - secs.begin();
			size_t litIndex = map.Sectors().GetSelectedSectorId();
			return (litIndex < nSectors) ? secs.begin()[litIndex] : nullptr;
		}

		void SetEditor(IEditorState* editor) override { this->editor = editor; }
		void CancelHilight() override { map.Sectors().SelectSector(-1); }
		void Free() override { delete this; }
	};
}

namespace HV
{
	ISectorEditor* CreateSectorEditor(Platform& platform, IWorldMap& map, Windows::IWindow& parent)
	{
		return new SectorEditor(platform, map, parent);
	}
}