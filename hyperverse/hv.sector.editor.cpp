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
	static const EventIdRef evPopulateActions = "editor.logic.actions.populate"_event;
	static const EventIdRef evSelectAction = "editor.logic.action.selected"_event;
	static const EventIdRef evSelectActionType = "editor.logic.action-type.selected"_event;
	static const EventIdRef evPopulateActionType = "editor.logic.action-type.populate"_event;
	static const EventIdRef evAddActionEnabler = "editor.logic.add_action_enabler"_event;
	static const EventIdRef evRemoveActionEnabler = "editor.logic.remove_action_enabler"_event;
	static const EventIdRef evActionArgsEditor = "editor.logic.action-arguments"_event;

	struct ActionFactoryEnumerator : IStringVector
	{
		int32 Count() const override
		{
			return (int32) HV::ActionFactoryCount();
		}

		void GetItem(int32 item, char* text, size_t capacity) const override
		{
			cstr name = HV::GetActionFactory(item).Name();
			CopyString(text, capacity, name);
		}
	} s_ActionFactoryStrings;

	struct LogicEditor: public IObserver, IFieldEditorEventHandler
	{
		AutoFree<IFieldEditor> fieldEditor;

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

				sector->TriggersAndActions()[index].SetType((TriggerType)value);
			}

			int32 Count() const  override
			{
				return sector->TriggersAndActions().TriggerCount();
			}
		} triggerList;
		Platform& platform;
		ISectors& sectors;
		ISector* sector = nullptr;

		// The index of the action in the action editor
		int32 activeActionIndex = -1;

		AutoFree<IPaneBuilderSupervisor> logicPanel;

		LogicEditor(Platform& _platform, ISectors& _sectors) : 
			platform(_platform), sectors(_sectors)
		{
			logicPanel = platform.gui.BindPanelToScript("!scripts/hv/panel.logic.sxy");
			
			FieldEditorContext fc{ platform.publisher, platform.gui, platform.keyboard, *this };
			fieldEditor = CreateFieldEditor(fc);
			platform.publisher.Subscribe(this, evEditSectorLogic);
			platform.publisher.Subscribe(this, evUIInvoke);
			platform.publisher.Subscribe(this, evPopulateTriggers);
			platform.publisher.Subscribe(this, evPopulateActions);
			platform.publisher.Subscribe(this, evSelectAction);
			platform.publisher.Subscribe(this, evSelectActionType);
			platform.publisher.Subscribe(this, evPopulateActionType);
			platform.publisher.Subscribe(this, evAddActionEnabler);
			platform.publisher.Subscribe(this, evRemoveActionEnabler);
			platform.publisher.Subscribe(this, evActionArgsEditor);

			platform.gui.RegisterPopulator(evActionArgsEditor.name, &fieldEditor->UIElement());
		}

		~LogicEditor()
		{
			platform.publisher.Unsubscribe(this);
			platform.gui.UnregisterPopulator(&fieldEditor->UIElement());
		}

		void OnActiveIndexChanged(int32 index, const char* stringRepresentation) override
		{
			int32 triggerIndex = triggerList.GetActiveIndex();
			if (triggerIndex >= 0 && triggerIndex < triggerList.Count())
			{
				auto& trigger = sector->TriggersAndActions()[triggerIndex];
				auto& actions = trigger.Actions();
				if (activeActionIndex >= 0 && activeActionIndex < actions.Count())
				{
					actions[activeActionIndex].SetParameter(index, stringRepresentation);
				}
			}
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
				fieldEditor->Deactivate();
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
			sector->TriggersAndActions().RemoveAction(triggerList.activeIndex, activeActionIndex);
		}

		void SelectAction(int32 actionIndex)
		{
			activeActionIndex = actionIndex;
		}

		void OnEvent(Event& ev) override
		{
			if (ev == evEditSectorLogic)
			{
				OpenEditor();
			}
			else if (ev == evSelectAction)
			{
				auto& selectEvent = As<TEventArgs<int>>(ev);
				SelectAction(selectEvent.data);
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
			else if (ev == evPopulateActions)
			{
				auto& pop = As<TEventArgs<Rococo::IStringVector*>>(ev);

				int32 triggerIndex = triggerList.GetActiveIndex();
				if (triggerIndex >= 0 && triggerIndex < triggerList.Count())
				{
					pop.data = &sector->TriggersAndActions()[triggerIndex].GetStringVector();
				}
			}
			else if (ev == evPopulateActionType)
			{
				auto& pop = As<TEventArgs<Rococo::IStringVector*>>(ev);
				int32 triggerIndex = triggerList.GetActiveIndex();
				if (triggerIndex >= 0 && triggerIndex < triggerList.Count())
				{
					auto& trigger = sector->TriggersAndActions()[triggerIndex];
					auto& actions = trigger.Actions();
					if (activeActionIndex >= 0 && activeActionIndex < actions.Count())
					{
						pop.data = &s_ActionFactoryStrings;
					}
				}
			}
			else if (ev == evAddActionEnabler)
			{
				auto& isEnabled = As <TEventArgs<bool>>(ev);
				int32 triggerIndex = triggerList.GetActiveIndex();
				isEnabled.data = (triggerIndex >= 0 && triggerIndex < triggerList.Count());
			}
			else if (ev == evRemoveActionEnabler)
			{
				auto& isEnabled = As <TEventArgs<bool>>(ev);
				isEnabled.data = false;

				int32 triggerIndex = triggerList.GetActiveIndex();
				if (triggerIndex >= 0 && triggerIndex < triggerList.Count())
				{
					auto& actions = sector->TriggersAndActions()[triggerIndex].Actions();
					if (activeActionIndex >= 0 && activeActionIndex < actions.Count())
					{
						isEnabled.data = true;
					}
				}
			}
			else if (ev == evSelectActionType)
			{
				auto& selectionArgs = As<TEventArgs<int>>(ev);
				int32 typeIndex = selectionArgs.data;

				if (typeIndex >= 0 && typeIndex < ActionFactoryCount())
				{
					int32 triggerIndex = triggerList.GetActiveIndex();
					if (triggerIndex >= 0 && triggerIndex < triggerList.Count())
					{
						auto& trigger = sector->TriggersAndActions()[triggerIndex];
						auto& actions = trigger.Actions();
						if (activeActionIndex >= 0 && activeActionIndex < actions.Count())
						{
							trigger.Actions().SetAction(activeActionIndex, GetActionFactory(typeIndex), sector->AFCC());
							auto& action = trigger.Actions()[activeActionIndex];
							ShowActionArgsInFieldEditor(action);
						}
					}
				}
			}
		}

		void ShowActionArgsInFieldEditor(IAction& action)
		{
			fieldEditor->Clear();

			for (int i = 0; i < action.ParameterCount(); ++i)
			{
				ParamDesc desc = action.GetParameterName(i);

				ParameterBuffer buf;
				action.GetParameter(i, buf);

				switch (desc.type)
				{
				case PARAMETER_TYPE_INT_HEX:
				{
					int32 value = atoi(buf);
					fieldEditor->AddInt32FieldUnbounded(desc.name, value, true);
					break;
				}
				case PARAMETER_TYPE_INT_UNBOUNDED:
				{
					int32 value = atoi(buf);
					fieldEditor->AddInt32FieldUnbounded(desc.name, value, false);
					break;
				}
				case PARAMETER_TYPE_INT:
				{
					int32 value = atoi(buf);
					fieldEditor->AddInt32FieldBounded(desc.name, value, (int32)desc.minValue, (int32)desc.maxValue);
					break;
				}
				case PARAMETER_TYPE_GLOBALVAR_NAME:
					fieldEditor->AddStringField(desc.name, buf, 24);
					break;
				case PARAMETER_TYPE_EVENT_NAME:
					fieldEditor->AddStringField(desc.name, buf, 24);
					break;
				case PARAMETER_TYPE_SECTOR_STRING:
					fieldEditor->AddStringField(desc.name, buf, 24);
					break;
				case PARAMETER_TYPE_FLOAT:
				{
					float value = (float)atof(buf);
					fieldEditor->AddFloat32FieldBounded(desc.name, value, desc.minValue, desc.maxValue);
				}
				default:
					break;
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