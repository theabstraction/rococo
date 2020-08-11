#include "hv.h"
#include <rococo.mplat.h>
#include <rococo.strings.h>
#include <rococo.rings.inl>
#include <vector>

namespace
{
	using namespace HV;

	static const EventIdRef evEditTagsScrollTo =  "editor.tags.scroll-to"_event;
	static const EventIdRef evEditSectorLogic = "edit.sector.logic"_event;
	static const EventIdRef evEditSectorTags = "edit.sector.tags"_event;
	static const EventIdRef evSelectTag = "editor.tags.selected"_event;
	static const EventIdRef evPopulateTag = "editor.tag"_event;
	static const EventIdRef evPopulateTriggers = "editor.logic.enum_triggers"_event;
	static const EventIdRef evPopulateActions = "editor.logic.actions.populate"_event;
	static const EventIdRef evSelectAction = "editor.logic.action.selected"_event;
	static const EventIdRef evSelectActionType = "editor.logic.action-type.selected"_event;
	static const EventIdRef evPopulateActionType = "editor.logic.action-type.populate"_event;
	static const EventIdRef evAddActionEnabler = "editor.logic.add_action_enabler"_event;
	static const EventIdRef evRemoveActionEnabler = "editor.logic.remove_action_enabler"_event;
	static const EventIdRef evActionArgsEditor = "editor.logic.action-arguments"_event;
	static const EventIdRef evScrollToActionType = "editor.logic.action-arguments.scroll-to"_event;
	static const EventIdRef evScrollToAction = "editor.logic.action.scroll-to"_event;
	static const EventIdRef evTagsPopulate = "editor.tags.populate"_event;

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
		AutoFree<IFieldEditor> tagsEditor;

		struct TriggerList: IEnumVector
		{
			ISector* sector = nullptr;
			int32 activeIndex = 0;
			IFieldEditor* fieldEditor;

			int32 GetActiveIndex() const override
			{
				return activeIndex;
			}

			void SetActiveIndex(int32 index) override
			{
				fieldEditor->Clear();
				fieldEditor->Deactivate();
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
		AutoFree<IPaneBuilderSupervisor> tagsPanel;

		struct TagEvents: IFieldEditorEventHandler
		{
			LogicEditor* editor;
			void OnActiveIndexChanged(int32 index, const char* stringRepresentation) override
			{
				editor->sector->SetTag(editor->tagIndex, stringRepresentation);
			}
		} tagEvents;

		LogicEditor(Platform& _platform, ISectors& _sectors) : 
			platform(_platform), sectors(_sectors)
		{
			logicPanel = platform.gui.BindPanelToScript("!scripts/hv/panel.logic.sxy");
			tagsPanel = platform.gui.BindPanelToScript("!scripts/hv/panel.tags.sxy");

			ID_FONT idFont = platform.utilities.GetHQFonts().GetSysFont(Graphics::HQFont_EditorFont);
			
			FieldEditorContext fcActions { platform.publisher, platform.gui, platform.keyboard, *this, idFont };
			fieldEditor = CreateFieldEditor(fcActions);

			FieldEditorContext fcTags { platform.publisher, platform.gui, platform.keyboard, tagEvents, idFont };
			tagsEditor = CreateFieldEditor(fcTags);
			triggerList.fieldEditor = fieldEditor;

			tagEvents.editor = this;

			platform.publisher.Subscribe(this, evEditSectorLogic);
			platform.publisher.Subscribe(this, evEditSectorTags);
			platform.publisher.Subscribe(this, evUIInvoke);
			platform.publisher.Subscribe(this, evPopulateTriggers);
			platform.publisher.Subscribe(this, evPopulateActions);
			platform.publisher.Subscribe(this, evSelectAction);
			platform.publisher.Subscribe(this, evSelectActionType);
			platform.publisher.Subscribe(this, evPopulateActionType);
			platform.publisher.Subscribe(this, evAddActionEnabler);
			platform.publisher.Subscribe(this, evRemoveActionEnabler);
			platform.publisher.Subscribe(this, evTagsPopulate);
			platform.publisher.Subscribe(this, evSelectTag);

			platform.gui.RegisterPopulator(evActionArgsEditor.name, &fieldEditor->UIElement());
			platform.gui.RegisterPopulator(evPopulateTag.name, &tagsEditor->UIElement());
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

		void OpenTags()
		{
			auto id = sectors.GetSelectedSectorId();
			if (id != (size_t)-1)
			{
				sector = sectors.begin()[id];
				platform.gui.PushTop(tagsPanel->Supervisor(), true);
			}
			else
			{
				sector = nullptr;
			}
		}

		void CloseTags()
		{
			if (platform.gui.Top() == tagsPanel->Supervisor())
			{
				platform.gui.Pop();
			}
			else
			{
				Throw(0, "LogicEditor::CloseTags -> the tag panel was not on top of the gui stack");
			}
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
			fieldEditor->Clear();
			fieldEditor->Deactivate();
			sector->TriggersAndActions().AddTrigger(triggerList.activeIndex);
		}

		void RemoveTrigger()
		{
			fieldEditor->Clear();
			fieldEditor->Deactivate();
			if (triggerList.activeIndex >= 0 && triggerList.activeIndex < triggerList.Count())
			{
				sector->TriggersAndActions().RemoveTrigger(triggerList.activeIndex);
			}
		}

		void AddAction()
		{
			fieldEditor->Clear();
			fieldEditor->Deactivate();
			sector->TriggersAndActions().AddAction(triggerList.activeIndex);
		}

		void RemoveAction()
		{
			fieldEditor->Clear();
			fieldEditor->Deactivate();
			sector->TriggersAndActions().RemoveAction(triggerList.activeIndex, activeActionIndex);
		}

		void LowerAction()
		{
			fieldEditor->Clear();
			fieldEditor->Deactivate();

			int32 triggerIndex = triggerList.GetActiveIndex();
			if (triggerIndex >= 0 && triggerIndex < triggerList.Count())
			{
				auto& actions = sector->TriggersAndActions()[triggerIndex].Actions();
				if (activeActionIndex >= 0 && activeActionIndex < actions.Count())
				{
					if (activeActionIndex >= 0 && activeActionIndex < actions.Count() - 1)
					{
						actions.Swap(activeActionIndex, activeActionIndex + 1);
						activeActionIndex++;

						TEventArgs<int32> lineNumber;
						lineNumber.value = activeActionIndex;
						platform.publisher.Publish(lineNumber, evScrollToAction);
					}
				}
			}
		}

		void RaiseAction()
		{
			fieldEditor->Clear();
			fieldEditor->Deactivate();

			int32 triggerIndex = triggerList.GetActiveIndex();
			if (triggerIndex >= 0 && triggerIndex < triggerList.Count())
			{
				auto& actions = sector->TriggersAndActions()[triggerIndex].Actions();
				if (activeActionIndex >= 0 && activeActionIndex < actions.Count())
				{
					if (activeActionIndex > 0)
					{
						actions.Swap(activeActionIndex, activeActionIndex - 1);
						activeActionIndex--;

						TEventArgs<int32> lineNumber;
						lineNumber.value = activeActionIndex;
						platform.publisher.Publish(lineNumber, evScrollToAction);
					}
				}
			}
		}

		void SelectAction(int32 actionIndex)
		{
			activeActionIndex = actionIndex;

			int32 triggerIndex = triggerList.GetActiveIndex();
			if (triggerIndex >= 0 && triggerIndex < triggerList.Count())
			{
				auto& actions = sector->TriggersAndActions()[triggerIndex].Actions();
				if (actionIndex >= 0 && actionIndex < actions.Count())
				{
					auto& a = actions[actionIndex];

					for (size_t i = 0; i < ActionFactoryCount(); ++i)
					{
						if (&GetActionFactory(i) == &a.Factory())
						{
							TEventArgs<int32> lineNumber;
							lineNumber.value = (int32) i;
							platform.publisher.Publish(lineNumber, evScrollToActionType);
							ShowActionArgsInFieldEditor(a);
							break;
						}
					}
				}
			}
		}

		int32 tagIndex = -1;

		void SelectTag(int32 index)
		{
			this->tagIndex = index;

			tagsEditor->Clear();
			tagsEditor->Deactivate();

			char text[32];
			sector->GetTags().GetItem(index, text, sizeof text);
			if (Eq(text, GET_UNDEFINDED_TAG()))
			{
				text[0] = 0;
			}
			tagsEditor->AddStringField("tag", text, sizeof text, true);
		}

		void OnEvent(Event& ev) override
		{
			if (ev == evEditSectorLogic)
			{
				OpenEditor();
			}
			else if (ev == evEditSectorTags)
			{
				OpenTags();
			}
			else if (ev == evSelectAction)
			{
				auto& selectEvent = As<TEventArgs<int>>(ev);
				SelectAction(selectEvent.value);
			}
			else if (ev == evSelectTag)
			{
				auto& selectTag = As<TEventArgs<int>>(ev);
				SelectTag(selectTag.value);
			}
			else if (ev == Rococo::Events::evUIInvoke)
			{
				auto& cmd = As<UIInvoke>(ev);
				if (Eq(cmd.command, "editor.logic.close"))
				{
					CloseEditor();
				}
				if (Eq(cmd.command, "editor.tags.close"))
				{
					CloseTags();
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
				else if (Eq(cmd.command, "editor.logic.raise_action"))
				{
					RaiseAction();
				}
				else if (Eq(cmd.command, "editor.logic.lower_action"))
				{
					LowerAction();
				}
				else if (Eq(cmd.command, "editor.tags.raise"))
				{
					sector->RaiseTag(tagIndex);
					if (tagIndex > 0) tagIndex--;
					TEventArgs<int32> line;
					line.value = tagIndex;
					platform.publisher.Publish(line, evEditTagsScrollTo);
				}
				else if (Eq(cmd.command, "editor.tags.lower"))
				{
					sector->LowerTag(tagIndex++);
					if (tagIndex >= sector->TagCount()) tagIndex--;
					TEventArgs<int32> line;
					line.value = tagIndex;
					platform.publisher.Publish(line, evEditTagsScrollTo);
				}
				else if (Eq(cmd.command, "editor.tags.add"))
				{
					sector->AddTag(tagIndex, GET_UNDEFINDED_TAG());
					if (tagIndex < 0 || tagIndex >= sector->GetTags().Count())
					{
						SelectTag(0);
					}

					TEventArgs<int32> lineZero;
					lineZero.value = 0;
					platform.publisher.Publish(lineZero, evEditTagsScrollTo);
				}
				else if (Eq(cmd.command, "editor.tags.remove"))
				{
					if (tagIndex >= 0 && tagIndex < sector->GetTags().Count())
					{
						sector->RemoveTag(tagIndex);
					}
				}
			}
			else if (ev == evPopulateTriggers)
			{
				auto& pop = As<TEventArgs<Rococo::IEnumVector*>>(ev);
				if (triggerList.sector != nullptr)
				{
					pop.value = &triggerList;
				}
			}
			else if (ev == evPopulateActions)
			{
				auto& pop = As<T2EventArgs<Rococo::IStringVector*,int32>>(ev);

				int32 triggerIndex = triggerList.GetActiveIndex();
				if (triggerIndex >= 0 && triggerIndex < triggerList.Count())
				{
					pop.value1 = &sector->TriggersAndActions()[triggerIndex].GetStringVector();
					pop.value2 = activeActionIndex;
				}
			}
			else if (ev == evTagsPopulate)
			{
				auto& pop = As<T2EventArgs<Rococo::IStringVector*, int32>>(ev);

				pop.value1 = &sector->GetTags();
				pop.value2 = -1;
			}
			else if (ev == evPopulateActionType)
			{
				auto& pop = As<T2EventArgs<Rococo::IStringVector*,int32>>(ev);
				int32 triggerIndex = triggerList.GetActiveIndex();
				if (triggerIndex >= 0 && triggerIndex < triggerList.Count())
				{
					auto& trigger = sector->TriggersAndActions()[triggerIndex];
					auto& actions = trigger.Actions();
					if (activeActionIndex >= 0 && activeActionIndex < actions.Count())
					{
						pop.value1 = &s_ActionFactoryStrings;
						pop.value2 = activeActionIndex;
					}
				}
			}
			else if (ev == evAddActionEnabler)
			{
				auto& isEnabled = As <TEventArgs<bool>>(ev);
				int32 triggerIndex = triggerList.GetActiveIndex();
				isEnabled.value = (triggerIndex >= 0 && triggerIndex < triggerList.Count());
			}
			else if (ev == evRemoveActionEnabler)
			{
				auto& isEnabled = As <TEventArgs<bool>>(ev);
				isEnabled.value = false;

				int32 triggerIndex = triggerList.GetActiveIndex();
				if (triggerIndex >= 0 && triggerIndex < triggerList.Count())
				{
					auto& actions = sector->TriggersAndActions()[triggerIndex].Actions();
					if (activeActionIndex >= 0 && activeActionIndex < actions.Count())
					{
						isEnabled.value = true;
					}
				}
			}
			else if (ev == evSelectActionType)
			{
				auto& selectionArgs = As<TEventArgs<int>>(ev);
				int32 typeIndex = selectionArgs.value;

				fieldEditor->Clear();
				fieldEditor->Deactivate();

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
			fieldEditor->Deactivate();

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
					fieldEditor->AddStringField(desc.name, buf, 24, true);
					break;
				case PARAMETER_TYPE_EVENT_NAME:
					fieldEditor->AddStringField(desc.name, buf, 24, true);
					break;
				case PARAMETER_TYPE_SECTOR_STRING:
					fieldEditor->AddStringField(desc.name, buf, 24, true);
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
		LogicEditor* logicEditor = nullptr;

		void Render(IGuiRenderContext& grc, const GuiRect& rect) override
		{
			if (!logicEditor)
			{
				logicEditor = new LogicEditor(platform, map.Sectors());
			}
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

			return true;
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
			menu.AddString(0, "Tags..."_fstring, "edit.sector.tags"_fstring, ""_fstring);
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
									if (!s->IsDirty()) PopupSectorContext(cursorPos, (int32) i);
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
			editor(nullptr)
		{ 
		}

		~SectorEditor()
		{
			delete logicEditor;
		}

		IEditMode& Mode() override { return *this; }
		const ISector* GetHilight() const override
		{
			auto& secs = map.Sectors();
			size_t nSectors = secs.end() - secs.begin();
			size_t litIndex = map.Sectors().GetSelectedSectorId();
			return (litIndex < nSectors) ? secs.begin()[litIndex] : nullptr;
		}

		void SetEditor(IEditorState* editor) override
		{ 
			this->editor = editor;
		}

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