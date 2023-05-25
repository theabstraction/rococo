#include <rococo.gui.retained.ex.h>
#include <vector>
#include <stdio.h>
#include <string>
#include <algorithm>
#include <rococo.maths.i32.h>
#include <unordered_map>
#include <unordered_set>
#include <rococo.maths.h>

using namespace Rococo;
using namespace Rococo::Gui;

namespace Rococo::Gui
{
	IGRLayoutSupervisor* CreateFullScreenLayout();
	IGRWidgetMainFrame* CreateGRMainFrame(cstr name, IGRPanel& panel);
	IGRPanelSupervisor* CreatePanel(IGRPanelRoot& root, IGRPanelSupervisor* parent);

	bool operator == (const GuiRect& a, const GuiRect& b)
	{
		return a.left == b.left && a.right == b.right && a.top == b.top && a.bottom == b.bottom;
	}
}

namespace ANON
{
	struct GuiRetained: IGuiRetainedSupervisor, IGRPanelRoot
	{
		IGRCustodian& custodian;
		GRConfig config;
		AutoFree<ISchemeSupervisor> scheme = CreateScheme();
		std::unordered_map<int64, IGRPanel*> mapIdToPanel;
		int queryDepth = 0;
		bool queueGarbageCollect = false;

		int badSpanCountThisFrame = 0;

		int BadSpanCount() const override
		{
			if (Rococo::OS::IsDebugging())
			{
				Rococo::OS::TripDebugger();
			}
			return badSpanCountThisFrame;
		}

		void IncBadSpanCountThisFrame(IGRPanel& origin) override
		{
			badSpanCountThisFrame++;
		}

		struct FrameDesc
		{
			IGRPanelSupervisor* panel;
			IGRWidgetMainFrame* frame;
			std::string id;

			bool Eq(IdWidget other) const
			{
				return other.Name && strcmp(other.Name, id.c_str()) == 0;
			}
		};

		using TFrames = std::vector<FrameDesc>;

		TFrames frameDescriptors;

		GuiRetained(GRConfig& _config, IGRCustodian& _custodian) : config(_config), custodian(_custodian)
		{

		}

		virtual ~GuiRetained()
		{
			for (auto d : frameDescriptors)
			{
				d.panel->Free();
			}
		}

		void QueueGarbageCollect() override
		{
			queueGarbageCollect = true;
		}

		IGRWidget* FindWidget(int64 panelId)
		{
			auto i = mapIdToPanel.find(panelId);
			return i != mapIdToPanel.end() ? &i->second->Widget() : nullptr;
		}

		void NotifyPanelDeleted(int64 uniqueId)
		{
			mapIdToPanel.erase(uniqueId);
		}

		void Free() override
		{
			delete this;
		}

		IGRMainFrame& BindFrame(IdWidget id) override
		{
			if (queryDepth > 0)
			{
				custodian.RaiseError(GRErrorCode::RecursionLocked, __FUNCTION__, "The GUI Retained API is locked for a recursive query. BindFrame cannot be executed here");
				IGRMainFrame* frame = nullptr;
				return *frame;
			}

			IGRMainFrame* oldFrame = FindFrame(id);
			if (oldFrame)
			{
				return *oldFrame;
			}

			// Create the frame descriptor string first, so we can pass a const reference to the internal string to the frame
			frameDescriptors.push_back(FrameDesc{ nullptr, nullptr, id.Name });

			auto& last = frameDescriptors.back();

			auto* newFramePanel = CreatePanel(*this, nullptr);
			auto* newFrame = CreateGRMainFrame(last.id.c_str(), * newFramePanel);
			newFramePanel->SetWidget(*newFrame);
			last.panel = newFramePanel;
			last.frame = newFrame;

			mapIdToPanel.try_emplace(newFramePanel->Id(), newFramePanel);
			return newFrame->Frame();
		}

		void DeleteFrame(IdWidget id) override
		{
			if (queryDepth > 0)
			{
				custodian.RaiseError(GRErrorCode::RecursionLocked, __FUNCTION__, "The GUI Retained API is locked for a recursive query. FrameDelete cannot be executed here");
				return;
			}

			auto matchesId = [&id](const FrameDesc& desc)
			{
				return desc.Eq(id);
			};

			auto d = std::remove_if(frameDescriptors.begin(), frameDescriptors.end(), matchesId);
			frameDescriptors.erase(d, frameDescriptors.end());
		}

		IScheme& Scheme()
		{
			return *scheme;
		}

		IGRMainFrame* FindFrame(IdWidget id) override
		{
			for (auto& d : frameDescriptors)
			{
				if (d.Eq(id))
				{
					return &d.frame->Frame();
				}
			}

			return nullptr;
		}

		GuiRect lastLayedOutScreenDimensions { 0,0,0,0 };

		struct RecursionGuard
		{
			GuiRetained& This;

			RecursionGuard(GuiRetained& _This) : This(_This)
			{
				This.queryDepth++;
			}
			~RecursionGuard()
			{
				This.queryDepth--;
			}
		};

		void LayoutFrames()
		{
			RecursionGuard guard(*this);
			for (auto& d : frameDescriptors)
			{
				d.panel->Resize(Span(lastLayedOutScreenDimensions));
				d.panel->LayoutRecursive({ 0,0 });
			}
		}

		void GarbageCollect() override
		{
			if (queryDepth > 0)
			{
				custodian.RaiseError(GRErrorCode::RecursionLocked, __FUNCTION__, "The GUI Retained API is locked for a recursive query. GarbageCollect cannot be executed here");
				return;
			}

			RecursionGuard guard(*this);
			for (auto& d : frameDescriptors)
			{
				d.panel->GarbageCollectRecursive();
			}
		}

		std::unordered_set<int64> invalidatedPanelSet;

		void UpdateNextFrame(IGRPanel& panel)
		{
			invalidatedPanelSet.insert(panel.Id());
		}

		void RenderGui(IGRRenderContext& g) override
		{
			if (queueGarbageCollect)
			{
				GarbageCollect();
				queueGarbageCollect = false;
			}

			badSpanCountThisFrame = 0;

			for (auto& d : frameDescriptors)
			{
				auto screenDimensions = g.ScreenDimensions();
				Vec2i topLeft = { screenDimensions.left, screenDimensions.top };

				if (lastLayedOutScreenDimensions != screenDimensions || d.panel->RequiresLayout())
				{
					invalidatedPanelSet.clear(); // Layout frames will operate on all panels, so this set becomes superfluous in this case
					lastLayedOutScreenDimensions = screenDimensions;
					LayoutFrames();
				}
				else
				{
					while (!invalidatedPanelSet.empty())
					{
						auto i = invalidatedPanelSet.begin();
						int64 panelId = *invalidatedPanelSet.begin();
						auto* invalidatedWidget = FindWidget(panelId);
						if (invalidatedWidget)
						{
							GuiRect absRect = invalidatedWidget->Panel().AbsRect();
							static_cast<IGRPanelSupervisor&>(invalidatedWidget->Panel()).LayoutRecursive(TopLeft(absRect));
						}
						invalidatedPanelSet.erase(panelId);
					}
				}

				RecursionGuard guard(*this);
				d.panel->RenderRecursive(g, screenDimensions);
				g.DisableScissors();
			}

			if (focusId >= 0)
			{
				auto* widget = FindWidget(focusId);
				GuiRect rect = widget->Panel().AbsRect();
				if (rect.right > rect.left && rect.bottom > rect.top)
				{
					RGBAb colour = widget->Panel().GetColour(ESchemeColourSurface::FOCUS_RECTANGLE, RGBAb(255, 255, 255, 255));
					g.DrawRectEdge(rect, colour, colour);
				}
			}
		}

		void MakeFirstToRender(IdWidget id) override
		{
			auto matchesId = [&id](const FrameDesc& desc)
			{
				return desc.Eq(id);
			};

			auto i = std::find_if(frameDescriptors.begin(), frameDescriptors.end(), matchesId);
			if (i != frameDescriptors.end())
			{
				TFrames::iterator firstOne = frameDescriptors.begin();
				std::swap(i, firstOne);
			}
		}

		void MakeLastToRender(IdWidget id) override
		{
			auto matchesId = [&id](const FrameDesc& desc)
			{
				return desc.Eq(id);
			};
			
			auto i = std::find_if(frameDescriptors.begin(), frameDescriptors.end(), matchesId);
			if (i != frameDescriptors.end())
			{
				TFrames::iterator lastOne = frameDescriptors.begin();
				std::advance(i, frameDescriptors.size() - 1);
				std::swap(i, lastOne);
			}
		}

		IGRPanelRoot& Root() override
		{
			return *this;
		}

		IGRWidget& AddWidget(IGRPanel& parent, IGRWidgetFactory& factory)
		{
			auto& panel = parent.AddChild();
			auto& widget = factory.CreateWidget(panel);
			auto& superPanel = static_cast<IGRPanelSupervisor&>(panel);
			superPanel.SetWidget(widget);
			mapIdToPanel.try_emplace(panel.Id(), &panel);
			return widget;
		}

		IGuiRetained& GR()
		{
			return *this;
		}

		bool isVisible = true;

		void SetVisible(bool isVisible) override
		{
			this->isVisible = isVisible;
		}

		bool IsVisible() const override
		{
			return isVisible && !frameDescriptors.empty();
		}

		EventRouting RouteCursorClickEvent(CursorEvent& ev) override
		{
			RecursionGuard guard(*this);

			if (captureId >= 0)
			{
				auto* widget = FindWidget(captureId);
				if (widget)
				{
					auto& panelSupervisor = static_cast<IGRPanelSupervisor&>(widget->Panel());
					if (panelSupervisor.RouteCursorClickEvent(ev, false) == EventRouting::NextHandler)
					{
						return widget->OnCursorClick(ev);
					}
					else
					{
						return EventRouting::Terminate;
					}
				}
			}

			for (auto d = frameDescriptors.rbegin(); d != frameDescriptors.rend(); ++d)
			{
				auto routing = d->panel->RouteCursorClickEvent(ev, true);
				if (routing == EventRouting::Terminate)
				{
					return EventRouting::Terminate;
				}
			}

			return EventRouting::NextHandler;
		}

		int64 captureId = -1;
		int64 focusId = -1;

		void CaptureCursor(IGRPanel& panel) override
		{
			captureId = panel.Id();
		}

		int64 CapturedPanelId() const override
		{
			return captureId;
		}

		void ReleaseCursor() override
		{
			captureId = -1;
		}

		typedef std::vector<PanelEvent> TPanelHistory;

		TPanelHistory movementCallstack;
		TPanelHistory previousMovementCallstack;

		class PanelEventBuilder : public IPanelEventBuilder
		{
			TPanelHistory& history;
		public:
			PanelEventBuilder(TPanelHistory& _history): history(_history)
			{
			}

			IPanelEventBuilder& operator += (const PanelEvent& ev) override
			{
				history.push_back(ev);
				return *this;
			}
		};

		void RouteOnLeaveEvent()
		{
			// Suppose our movement callstack at time t + dt is A, B, C; but our previous callstack at time t was A, E, F, then we need to notify E and F that the cursor has left their domain
			for (int i = 0; i < previousMovementCallstack.size(); ++i)
			{
				auto& previousEvent = previousMovementCallstack[i];

				if (i >= movementCallstack.size() || previousEvent.panelId != movementCallstack[i].panelId)
				{
					for (int j = i; j < previousMovementCallstack.size(); ++j)
					{
						IGRWidget* widget = FindWidget(previousMovementCallstack[j].panelId);
						if (widget)
						{
							widget->OnCursorLeave();
						}
					}

					return;
				}
			}
		}

		void RouteOnEnterEvent()
		{
			// Suppose our movement callstack at time t + dt is A, B, C; but our previous callstack at time t was A, E, then we need to notify that B and C that the cursor has entered their domain
			for (int i = 0; i < movementCallstack.size(); ++i)
			{
				auto& moveEvent = movementCallstack[i];
				if (i >= previousMovementCallstack.size() || moveEvent.panelId != previousMovementCallstack[i].panelId)
				{
					for (int j = i; j < movementCallstack.size(); ++j)
					{
						IGRWidget* widget = FindWidget(movementCallstack[j].panelId);
						if (widget)
						{
							widget->OnCursorEnter();
						}
					}

					return;
				}
			}
		}

		bool TryAppendWidgetsUnderCursorToMovementCallstack(TPanelHistory& callstack, CursorEvent& ev)
		{
			if (captureId >= 0)
			{
				auto* widget = FindWidget(captureId);
				if (widget)
				{
					callstack.push_back({ captureId, & widget->Panel(), widget->Panel().AbsRect() });
				}
				else
				{
					captureId = -1;
					return false;
				}
			}
			else
			{
				PanelEventBuilder pb(callstack);

				for (auto d = frameDescriptors.rbegin(); d != frameDescriptors.rend(); ++d)
				{
					d->panel->BuildCursorMovementHistoryRecursive(ev, pb);
					if (!callstack.empty())
					{
						break;
					}
				}
			}

			return true;
		}

		EventRouting RouteCursorMoveEvent(CursorEvent& ev) override
		{
			movementCallstack.clear();

			if (!TryAppendWidgetsUnderCursorToMovementCallstack(movementCallstack, ev) || movementCallstack.empty())
			{
				return EventRouting::Terminate;
			}

			EventRouting result = EventRouting::NextHandler;

			RecursionGuard guard(*this);

			for (auto i = movementCallstack.rbegin(); i != movementCallstack.rend(); ++i)
			{
				if (i->panel->Widget().OnCursorMove(ev) == EventRouting::Terminate)
				{
					result = EventRouting::Terminate;
				}
			}

			if (captureId == -1)
			{
				RouteOnLeaveEvent();
				RouteOnEnterEvent();
			}

			std::swap(movementCallstack, previousMovementCallstack);

			return result;
		}

		EventRouting RouteKeyEvent(KeyEvent& keyEvent) override
		{
			RecursionGuard guard(*this);

			if (focusId < 0)
			{
				return EventRouting::Terminate;
			}

			auto* widget = FindWidget(focusId);
			if (!widget)
			{
				return EventRouting::Terminate;
			}

			return widget->OnKeyEvent(keyEvent);
		}

		IGRCustodian& Custodian() override
		{
			return custodian;
		}

		int64 GetFocusId() const override
		{
			return focusId;
		}

		void SetFocus(int64 id = -1) override
		{
			focusId = id;
		}
	};
}

namespace Rococo
{
	// Copied from the maths lib. We want our DLL to depend on as few libraries as possible to make it easier to re-use in third party apps
	bool IsPointInRect(Vec2i p, const GuiRect& rect)
	{
		return (p.x >= rect.left && p.x <= rect.right && p.y >= rect.top && p.y <= rect.bottom);
	}
}

namespace Rococo::Gui
{
	ROCOCO_GUI_RETAINED_API IGuiRetainedSupervisor* CreateGuiRetained(GRConfig& config, IGRCustodian& custodian)
	{
		return new ANON::GuiRetained(config, custodian);
	}

	ROCOCO_GUI_RETAINED_API [[nodiscard]] GRAnchors GRAnchors::Left()
	{
		GRAnchors a;
		a.left = true;
		return a;
	}

	ROCOCO_GUI_RETAINED_API [[nodiscard]] GRAnchors GRAnchors::LeftAndRight()
	{
		GRAnchors a;
		a.left = true;
		a.right = true;
		return a;
	}

	ROCOCO_GUI_RETAINED_API [[nodiscard]] GRAnchors GRAnchors::ExpandAll()
	{
		GRAnchors a;
		a.left = true;
		a.right = true;
		a.top = true;
		a.bottom = true;
		a.expandsVertically = true;
		a.expandsHorizontally = true;
		return a;
	}

	ROCOCO_GUI_RETAINED_API [[nodiscard]] GRAnchors GRAnchors::Right()
	{
		GRAnchors a;
		a.right = true;
		return a;
	}

	ROCOCO_GUI_RETAINED_API [[nodiscard]] GRAnchors GRAnchors::Top()
	{
		GRAnchors a;
		a.top = true;
		return a;
	}

	ROCOCO_GUI_RETAINED_API [[nodiscard]] GRAnchors GRAnchors::Bottom()
	{
		GRAnchors a;
		a.bottom = true;
		return a;
	}

	ROCOCO_GUI_RETAINED_API [[nodiscard]] GRAnchors GRAnchors::TopAndBottom()
	{
		GRAnchors a;
		a.top = true;
		a.bottom = true;
		return a;
	}

	ROCOCO_GUI_RETAINED_API [[nodiscard]] GRAnchors GRAnchors::ExpandVertically()
	{
		GRAnchors a;
		a.expandsVertically = true;
		return a;
	}

	ROCOCO_GUI_RETAINED_API [[nodiscard]] GRAnchors GRAnchors::ExpandHorizontally()
	{
		GRAnchors a;
		a.expandsHorizontally = true;
		return a;
	}

	ROCOCO_GUI_RETAINED_API [[nodiscard]] bool DoInterfaceNamesMatch(cstr a, cstr b)
	{
		return _stricmp(a, b) == 0;
	}
}