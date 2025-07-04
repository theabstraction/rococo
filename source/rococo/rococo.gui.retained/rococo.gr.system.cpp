#include <rococo.gui.retained.ex.h>
#include <vector>
#include <stdio.h>
#include <string>
#include <algorithm>
#include <rococo.maths.i32.h>
#include <unordered_map>
#include <rococo.maths.h>
#include <rococo.ui.h>
#include <rococo.ringbuffer.h>

#define ROCOCO_USE_SAFE_V_FORMAT
#include <rococo.strings.h>

using namespace Rococo;
using namespace Rococo::Gui;

namespace Rococo::Gui
{
	IGRLayoutSupervisor* CreateFullScreenLayout();
	IGRWidgetMainFrameSupervisor* CreateGRMainFrame(cstr name, IGRPanel& panel);
	IGRPanelSupervisor* CreatePanel(IGRPanelRootSupervisor& root, IGRPanelSupervisor* parent);

	bool operator == (const GuiRect& a, const GuiRect& b)
	{
		return a.left == b.left && a.right == b.right && a.top == b.top && a.bottom == b.bottom;
	}
}

namespace ANON
{
	struct GRSystem: IGRSystemSupervisor, IGRPanelRootSupervisor
	{
		GRConfig config;
		IGRCustodian& custodian;
		AutoFree<IGRSchemeSupervisor> scheme = CreateGRScheme();
		std::unordered_map<int64, IGRPanel*> mapIdToPanel;
		int queryDepth = 0;
		bool queueGarbageCollect = false;

		int grDebugFlags = 0;

		bool HasDebugFlag(EGRDebugFlags flag) const override
		{
			return (grDebugFlags & (int) flag) != 0;
		}

		void SetDebugFlags(int grDebugFlags) override
		{
			this->grDebugFlags |= (int)grDebugFlags;
		}

		struct FrameDesc
		{
			IGRPanelSupervisor* panel;
			IGRWidgetMainFrame* frame;
			std::string id;

			bool Eq(GRIdWidget other) const
			{
				return other.Name && strcmp(other.Name, id.c_str()) == 0;
			}
		};

		using TFrames = std::vector<FrameDesc>;

		TFrames frameDescriptors;

		OneReaderOneWriterCircleBuffer<GRWidgetEvent>* eventQueue;
		OneReaderOneWriterCircleBuffer<GRWidgetEvent>* dispatchQueue;

		enum { MAX_EVENT_QUEUE_LENGTH = 1024 };

		GRSystem(GRConfig& _config, IGRCustodian& _custodian) :
			config(_config),
			custodian(_custodian), 
			eventQueue(new OneReaderOneWriterCircleBuffer<GRWidgetEvent>(MAX_EVENT_QUEUE_LENGTH)),
			dispatchQueue(new OneReaderOneWriterCircleBuffer<GRWidgetEvent>(MAX_EVENT_QUEUE_LENGTH))
		{

		}

		virtual ~GRSystem()
		{
			for (auto d : frameDescriptors)
			{
				d.panel->ReleasePanel();
			}

			delete eventQueue;
			delete dispatchQueue;
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

		IGRWidgetMainFrame& BindFrame(GRIdWidget id) override
		{
			if (queryDepth > 0)
			{
				custodian.RaiseError(nullptr, EGRErrorCode::RecursionLocked, __ROCOCO_FUNCTION__, "The GUI Retained API is locked for a recursive query. BindFrame cannot be executed here");
				IGRWidgetMainFrame* frame = nullptr;
				return *frame;
			}

			IGRWidgetMainFrame* oldFrame = FindFrame(id);
			if (oldFrame)
			{
				return *oldFrame;
			}

			// Create the frame descriptor string first, so we can pass a const reference to the internal string to the frame
			frameDescriptors.push_back(FrameDesc{ nullptr, nullptr, id.Name });

			auto& last = frameDescriptors.back();

			auto* newFramePanel = CreatePanel(*this, nullptr);
			auto* newFrame = CreateGRMainFrame(last.id.c_str(), * newFramePanel);
			newFramePanel->SetWidget(newFrame->WidgetSupervisor());
			last.panel = newFramePanel;
			last.frame = newFrame;

			mapIdToPanel.try_emplace(newFramePanel->Id(), newFramePanel);
			return *newFrame;
		}

		void DeleteFrame(GRIdWidget id) override
		{
			if (queryDepth > 0)
			{
				custodian.RaiseError(nullptr, EGRErrorCode::RecursionLocked, __ROCOCO_FUNCTION__, "The GUI Retained API is locked for a recursive query. FrameDelete cannot be executed here");
				return;
			}

			auto matchesId = [&id](const FrameDesc& desc)
			{
				return desc.Eq(id);
			};

			auto d = std::remove_if(frameDescriptors.begin(), frameDescriptors.end(), matchesId);
			frameDescriptors.erase(d, frameDescriptors.end());
		}

		IGRScheme& Scheme()
		{
			return *scheme;
		}

		Vec2i ScreenDimensions() const override
		{
			return Span(screenDimensions);
		}

		IGRWidgetMainFrame* FindFrame(GRIdWidget id) override
		{
			for (auto& d : frameDescriptors)
			{
				if (d.Eq(id))
				{
					return d.frame;
				}
			}

			return nullptr;
		}

		// Added to a method to prevent the API consumer from modifying the widget tree while the implementation expects the tree hierarchy to be immutable
		struct RecursionGuard
		{
			GRSystem& This;

			RecursionGuard(GRSystem& _This) : This(_This)
			{
				This.queryDepth++;
			}
			~RecursionGuard()
			{
				This.queryDepth--;
			}
		};

		void GarbageCollect() override
		{
			if (queryDepth > 0)
			{
				custodian.RaiseError(nullptr, EGRErrorCode::RecursionLocked, __ROCOCO_FUNCTION__, "The GUI Retained API is locked for a recursive query. GarbageCollect cannot be executed here");
				return;
			}

			RecursionGuard guard(*this);
			for (auto& d : frameDescriptors)
			{
				d.panel->GarbageCollectRecursive();
			}
		}

		Vec2i lastRenderedCursorPosition{ -10000000, -10000000 };

		std::vector<IGRPanelSupervisor*> deferredRenderQueue;

		void DeferRendering(IGRPanelSupervisor& panel) override
		{
			deferredRenderQueue.push_back(&panel);
		}

		GuiRect screenDimensions{ 0,0,0,0 };

		void RenderAllFrames(IGRRenderContext& g) override
		{
			lastRenderedCursorPosition = g.CursorHoverPoint();

			if (queueGarbageCollect)
			{
				GarbageCollect();
				queueGarbageCollect = false;
			}

			RecursionGuard guard(*this);

			screenDimensions = g.ScreenDimensions();

			for (auto& d : frameDescriptors)
			{
				d.panel->SetConstantWidth(Width(screenDimensions));
				d.panel->SetConstantHeight(Height(screenDimensions));
				d.panel->Layout();
			}

			for (auto& d : frameDescriptors)
			{
				d.panel->RenderRecursive(g, screenDimensions, true, focusId);
				g.DisableScissors();
			}

			for (auto* panel : deferredRenderQueue)
			{
				panel->RenderRecursive(g, screenDimensions, false, focusId);
				g.DisableScissors();
			}

			if (focusId >= 0)
			{
				auto* widget = FindFocusWidget();
				if (widget)
				{
					if (widget->Panel().IsCollapsedOrAncestorCollasped())
					{
						focusId = -1;
					}
					else
					{
						GuiRect rect = widget->Panel().AbsRect();
						if (rect.right > rect.left && rect.bottom > rect.top)
						{
							RGBAb colour = widget->Panel().GetColour(EGRSchemeColourSurface::FOCUS_RECTANGLE, GRWidgetRenderState(false, false, true), RGBAb(255, 255, 255, 255));
							g.DrawRectEdge(rect, colour, colour);
						}
					}
				}
			}

			deferredRenderQueue.clear();

			RenderDebugInfo(g);
		}

		IGRSystemSubRenderer* focusOverlayRenderer = nullptr;

		void RenderFocus(IGRPanel& panel, IGRRenderContext& g, const GuiRect& clipRect) override
		{
			if (focusOverlayRenderer)
			{
				focusOverlayRenderer->Render(panel, g, clipRect);
			}
		}

		void SetFocusOverlayRenderer(IGRSystemSubRenderer* subRenderer)
		{
			focusOverlayRenderer = subRenderer;
		}

		void RenderDebugInfo(IGRRenderContext& g)
		{
			Vec2i pos = g.CursorHoverPoint();

			GRFontId debugFontId = GRFontId::NONE;

			GRAlignmentFlags alignment;
			alignment.Add(EGRAlignment::Right);

			char cursorLine[32];
			Strings::SafeFormat(cursorLine, "%d %d", pos.x, pos.y);

			GuiRect debugRect;
			debugRect.left = screenDimensions.right - 70;
			debugRect.right = debugRect.left + 60;
			debugRect.top = screenDimensions.top + 10;
			debugRect.bottom = debugRect.top + 30;
			g.DrawRect(debugRect, RGBAb(0, 0, 0, 128));
			g.DrawText(debugFontId, debugRect, alignment, { 10, 0 }, to_fstring(cursorLine), RGBAb(255, 255, 255));
		}

		void MakeFirstToRender(GRIdWidget id) override
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

		void MakeLastToRender(GRIdWidget id) override
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
			superPanel.SetWidget(widget.Supervisor());
			mapIdToPanel.try_emplace(panel.Id(), &panel);
			return widget;
		}

		IGRSystem& GR()
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

		EGREventRouting RouteCursorClickEvent(GRCursorEvent& ev) override
		{
			RecursionGuard guard(*this);

			if (captureId >= 0)
			{
				auto* widget = FindWidget(captureId);
				if (widget)
				{
					auto& panelSupervisor = static_cast<IGRPanelSupervisor&>(widget->Panel());
					if (panelSupervisor.RouteCursorClickEvent(ev, false) == EGREventRouting::NextHandler)
					{
						return widget->Manager().OnCursorClick(ev);
					}
					else
					{
						return EGREventRouting::Terminate;
					}
				}
			}

			for (auto d = frameDescriptors.rbegin(); d != frameDescriptors.rend(); ++d)
			{
				auto routing = d->panel->RouteCursorClickEvent(ev, true);
				if (routing == EGREventRouting::Terminate)
				{
					return EGREventRouting::Terminate;
				}
			}

			return EGREventRouting::NextHandler;
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

		typedef std::vector<GRPanelEvent> TPanelHistory;

		TPanelHistory movementCallstack;
		TPanelHistory previousMovementCallstack;

		TPanelHistory keypressCallstack;

		class PanelEventBuilder : public IGRPanelEventBuilder
		{
			TPanelHistory& history;
		public:
			PanelEventBuilder(TPanelHistory& _history): history(_history)
			{
			}

			IGRPanelEventBuilder& operator += (const GRPanelEvent& ev) override
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
							widget->Manager().OnCursorLeave();
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
							widget->Manager().OnCursorEnter();
						}
					}

					return;
				}
			}
		}

		bool TryAppendWidgetsUnderCursorToMovementCallstack(TPanelHistory& callstack, GRCursorEvent& ev)
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

		bool TryAppendWidgetsUnderCursorCallstack(TPanelHistory& callstack)
		{
			if (captureId >= 0)
			{
				auto* widget = FindWidget(captureId);
				if (widget)
				{
					callstack.push_back({ captureId, &widget->Panel(), widget->Panel().AbsRect() });
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
					d->panel->BuildWidgetCallstackRecursiveUnderPoint(lastRenderedCursorPosition, pb);
					if (!callstack.empty())
					{
						break;
					}
				}
			}

			return true;
		}

		EGREventRouting RouteCursorMoveEvent(GRCursorEvent& ev) override
		{
			movementCallstack.clear();

			if (!TryAppendWidgetsUnderCursorToMovementCallstack(movementCallstack, ev) || movementCallstack.empty())
			{
				return EGREventRouting::Terminate;
			}

			EGREventRouting result = EGREventRouting::NextHandler;

			RecursionGuard guard(*this);

			if (movementCallstack.size() == 1 && movementCallstack[0].panelId == captureId)
			{
				auto* target = movementCallstack[0].panel;
				while (target)
				{
					if (target->Widget().Manager().OnCursorMove(ev) == EGREventRouting::Terminate)
					{
						break;
					}
					target = target->Parent();
				}
			}
			else
			{
				for (auto i = movementCallstack.rbegin(); i != movementCallstack.rend(); ++i)
				{
					if (i->panel->Widget().Manager().OnCursorMove(ev) == EGREventRouting::Terminate)
					{
						result = EGREventRouting::Terminate;
					}
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

		IGRWidgetSupervisor* TopMostFrame()
		{
			if (frameDescriptors.empty())
			{
				return nullptr;
			}

			return &frameDescriptors.back().frame->Widget().Supervisor();
		}

		EGREventRouting RouteKeyEventToPanelThenAncestors(IGRPanel& panel, GRKeyEvent& keyEvent)
		{
			auto result = panel.Widget().Manager().OnKeyEvent(keyEvent);
			if (result == EGREventRouting::Terminate)
			{
				return result;
			}

			for (auto* ancestor = panel.Parent(); ancestor != nullptr; ancestor = ancestor->Parent())
			{
				result = RouteKeyEventToPanelThenAncestors(*ancestor, keyEvent);
				if (result == EGREventRouting::Terminate)
				{
					return result;
				}
			}

			return EGREventRouting::NextHandler;
		}

		EGREventRouting RouteKeyEvent(GRKeyEvent& keyEvent) override
		{
			RecursionGuard guard(*this);

			if (focusId < 0)
			{
				IGRWidgetManager* frame = TopMostFrame();
				return frame ? frame->OnKeyEvent(keyEvent) : EGREventRouting::NextHandler;
			}

			auto* focusWidget = FindWidget(focusId);
			if (!focusWidget)
			{
				return EGREventRouting::Terminate;
			}

			return RouteKeyEventToPanelThenAncestors(focusWidget->Panel(), keyEvent);
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

		IGREventHandler* eventHandler = nullptr;

		IGREventHandler* SetEventHandler(IGREventHandler* eventHandler) override
		{
			IGREventHandler* oldEventHandler = eventHandler;
			this->eventHandler = eventHandler;
			return oldEventHandler;
		}

		EGREventRouting OnGREvent(GRWidgetEvent& ev) override
		{
			auto* back = eventQueue->GetBackSlot();
			if (!back)
			{
				GRWidgetEvent discarded;
				eventQueue->TryPopFront(OUT discarded);
				back = eventQueue->GetBackSlot();
				if (!back)
				{
					Throw(0, "%s: Expected back slot after front was popped", __ROCOCO_FUNCTION__);
				}
			}

			*back = ev;

			eventQueue->WriteBack();
			
			return EGREventRouting::Terminate;
		}

		void DispatchMessages() override
		{
			if (queryDepth > 0)
			{
				this->custodian.RaiseError(nullptr, EGRErrorCode::RecursionLocked, __ROCOCO_FUNCTION__, "Error, API consumer attempted to DispatchMessages from within a GR locked section.");
			}

			std::swap(dispatchQueue, eventQueue);

			GRWidgetEvent ev;
			while (dispatchQueue->TryPopFront(OUT ev))
			{
				eventHandler->OnGREvent(ev);
			}
		}

		GRRealtimeConfig realtimeConfig;

		const GRRealtimeConfig& Config() const override
		{
			return realtimeConfig;
		}

		GRRealtimeConfig& MutableConfig() override
		{
			return realtimeConfig;
		}

		[[nodiscard]] IGRFonts& Fonts() override
		{
			return custodian.Fonts();
		}

		void OnNavigate(EGRNavigationDirective directive)
		{
			auto* focusedWidget = FindWidget(focusId);
			if (!focusedWidget)
			{
				return;
			}

			for(IGRPanel* panel = &focusedWidget->Panel(); panel != nullptr; panel = panel->Parent())
			{
				IGRNavigator* navigator = Cast<IGRNavigator>(panel->Widget());
				if (navigator)
				{
					if (EGREventRouting::Terminate == navigator->OnNavigate(directive))
					{
						return;
					}
				}
			}
		}

		void ApplyKeyGlobally(GRKeyEvent& keyEvent) override
		{
			switch (keyEvent.osKeyEvent.VKey)
			{
			case IO::VirtualKeys::VKCode_TAB:
				if (keyEvent.osKeyEvent.IsUp())
				{
					if (focusId > 0)
					{
						OnNavigate(EGRNavigationDirective::Tab);
					}
				}
				break;
			}
		}
	};
}

namespace Rococo::Gui
{
	ROCOCO_GUI_RETAINED_API IGRSystemSupervisor* CreateGRSystem(GRConfig& config, IGRCustodian& custodian)
	{
		return new ANON::GRSystem(config, custodian);
	}

	ROCOCO_GUI_RETAINED_API EGREventRouting RouteEventToHandler(IGRPanel& panel, GRWidgetEvent& ev)
	{
		auto& supervisor = static_cast<IGRSystemSupervisor&>(panel.Root().GR());
		return supervisor.OnGREvent(ev);
	}

	ROCOCO_GUI_RETAINED_API [[nodiscard]] bool DoInterfaceNamesMatch(cstr a, cstr b)
	{
		return Strings::EqI(a, b);
	}

	static bool Is(cstr a, cstr b)
	{
		return Strings::EqI(a, b);
	}

	ROCOCO_GUI_RETAINED_API GRAlignmentFlags::GRAlignmentFlags(cstr textRepresentation)
	{
		if (textRepresentation == nullptr)
		{
			Rococo::Throw(0, "%s: (nullptr)", __ROCOCO_FUNCTION__);
		}

		if (Is(textRepresentation, "left"))
		{
			Add(EGRAlignment::Left);
			return;
		}

		if (Is(textRepresentation, "right"))
		{
			Add(EGRAlignment::Right);
			return;
		}

		if (Is(textRepresentation, "top"))
		{
			Add(EGRAlignment::Top);
			return;
		}

		if (Is(textRepresentation, "bottom"))
		{
			Add(EGRAlignment::Bottom);
			return;
		}

		if (Is(textRepresentation, "topleft"))
		{
			Add(EGRAlignment::Left).Add(EGRAlignment::Top);
			return;
		}

		if (Is(textRepresentation, "topright"))
		{
			Add(EGRAlignment::Right).Add(EGRAlignment::Top);;
			return;
		}

		if (Is(textRepresentation, "bottomleft"))
		{
			Add(EGRAlignment::Left).Add(EGRAlignment::Top);
			return;
		}

		if (Is(textRepresentation, "bottomright"))
		{
			Add(EGRAlignment::Right).Add(EGRAlignment::Top);;
			return;
		}

		if (Is(textRepresentation, "centre"))
		{
			return;
		}

		Rococo::Throw(0, "Expecting one of [Left, Right, Top, Bottom, TopLeft, TopRight, BottomLeft, BottomRight, Centre]");
	}

	ROCOCO_GUI_RETAINED_API IGRWidgetMainFrame* FindOwner(IGRWidget& widget)
	{
		IGRPanel* p = &widget.Panel(); 

		for (;;)
		{
			if (p->Parent() == nullptr)
			{
				return Cast<IGRWidgetMainFrame>(p->Widget());
			}

			p = p->Parent();
		}
	}

	ROCOCO_GUI_RETAINED_API [[nodiscard]] IGRWidgetManager& IGRWidget::Manager()
	{
		return static_cast<IGRWidgetManager&>(*this);
	}

	ROCOCO_GUI_RETAINED_API [[nodiscard]] IGRWidgetSupervisor& IGRWidget::Supervisor()
	{
		return static_cast<IGRWidgetSupervisor&>(*this);
	}
}