#include <rococo.api.h>
#include <rococo.mplat.h>
#define ROCOCO_USE_SAFE_V_FORMAT
#include <rococo.hashtable.h>
#include <rococo.ringbuffer.h>
#include <rococo.time.h>
#include <rococo.ui.h>
#include <mplat/mplat.events.h>
#include <vector>

using namespace Rococo;
using namespace Rococo::Events;
using namespace Rococo::Strings;
using namespace Rococo::Events;
using namespace Rococo::Graphics;

namespace Rococo
{
	namespace MPlatImpl
	{
		IPaneBuilderSupervisor* CreateDebuggingOverlay(Platform& platform);
		IPaneBuilderSupervisor* CreateScriptedPanel(Platform& platform, cstr filename, IScriptCompilationEventHandler* onCompile, IScriptEnumerator* implicitIncludes);
	}
}

class GuiStack : public IGuiStackSupervisor, public IObserver
{
	struct PanelDesc
	{
		IPaneSupervisor* panel;
		bool isModal;
	};
	std::vector<PanelDesc> panels;

	struct CommandHandler
	{
		HString helpString;
		ICommandHandler* handler;
		FN_OnCommand method;
	};

	IPublisher& publisher;
	ISourceCache& sourceCache;
	IRenderer& renderer;
	IUtilities& utilities;

	stringmap<CommandHandler> handlers;
	stringmap<IUIElement*> renderElements;

	IKeyboardSink* keyboardSink = nullptr;

	struct Message
	{
		char text[128];
	};

	struct VisibleMessage
	{
		Message message;
		int32 y;
	};

	OneReaderOneWriterCircleBuffer<Message> messageLog;
	std::vector<VisibleMessage> scrollingMessages;
public:
	Platform* platform = nullptr; // initialized in PostContruct

	GuiStack(IPublisher& _publisher, ISourceCache& _sourceCache, IRenderer& _renderer, IUtilities& _utilities) :
		publisher(_publisher),
		sourceCache(_sourceCache),
		renderer(_renderer),
		utilities(_utilities),
		messageLog(3)
	{
		publisher.Subscribe(this, evUIInvoke);
		publisher.Subscribe(this, evUIPopulate);
	}

	~GuiStack()
	{
		publisher.Unsubscribe(this);
	}

	void PostConstruct(Platform* platform) override
	{
		this->platform = platform;
	}

	void Free() override
	{
		delete this;
	}

	int Count() const override
	{
		return (int32) panels.size();
	}

	void AttachKeyboardSink(IKeyboardSink* ks) override
	{
		keyboardSink = ks;
	}

	void DetachKeyboardSink(IKeyboardSink* ks) override
	{
		if (ks == keyboardSink)
		{
			keyboardSink = nullptr;
		}
	}

	void LogMessage(const char* format, ...)
	{
		va_list argList;
		va_start(argList, format);

		Message message;
		SafeVFormat(message.text, sizeof(message), format, argList);

		auto* next = messageLog.GetBackSlot();
		while (next == nullptr)
		{
			Message front;
			messageLog.TryPopFront(front);
			next = messageLog.GetBackSlot();
		}

		*next = message;
		messageLog.WriteBack();
	}

	stringmap<std::vector<HString>> mapFileNameToErrorMessages;

	void ClearFileError(cstr file) override
	{
		auto i = mapFileNameToErrorMessages.find(file);
		if (i != mapFileNameToErrorMessages.end())
		{
			mapFileNameToErrorMessages.erase(i);
		}
	}

	void ShowFileError(cstr file, cstr message) override
	{
		auto i = mapFileNameToErrorMessages.find(file);
		if (i == mapFileNameToErrorMessages.end())
		{
			i = mapFileNameToErrorMessages.insert(file, std::vector<HString>()).first;
		}

		auto& messages = i->second;
		for (auto& m : messages)
		{
			if (Eq(m, message))
			{
				// Duplicate
				return;
			}
		}

		messages.push_back(message);
	}

	bool HighFrameRateImproves() const override
	{
		return !scrollingMessages.empty();
	}

	IKeyboardSink* CurrentKeyboardSink() override
	{
		return keyboardSink;
	}

	bool overwriting = false;

	bool IsOverwriting() const  override
	{
		return overwriting;
	}

	void ToggleOverwriteMode() override
	{
		overwriting = !overwriting;
	}

	void AppendEvent(const MouseEvent& me) override
	{
		platform->graphics.renderer.GuiResources().SetSysCursor(EWindowCursor_Default);

		if (panels.empty())
		{
			DirectMouseEvent dme(me);
			publisher.Publish(dme, evUIMouseEvent);
			if (dme.consumed) return;
		}

		for (auto i = panels.rbegin(); i != panels.rend(); ++i)
		{
			if (IsPointInRect(me.cursorPos, i->panel->ClientRect()))
			{
				i->panel->AppendEvent(me, { 0,0 });

				if (i->isModal)
				{
					break;
				}
			}
		}
	}

	bool AppendEvent(const KeyboardEventEx& ke) override
	{
		if (keyboardSink && keyboardSink->OnKeyboardEvent(ke))
		{
			return true;
		}

		if (ke.VKey == Rococo::IO::VirtualKeys::VKCode_INSERT && ke.IsUp())
		{
			ToggleOverwriteMode();
		}

		GuiMetrics metrics;
		renderer.GetGuiMetrics(metrics);

		for (auto i = panels.rbegin(); i != panels.rend(); ++i)
		{
			if (IsPointInRect(metrics.cursorPosition, i->panel->ClientRect()))
			{
				if (i->panel->AppendEvent(ke, metrics.cursorPosition, { 0,0 }))
				{
					return true;
				}
			}
		}

		return false;
	}

	void OnEvent(Event& ev) override
	{
		if (ev == evUIInvoke)
		{
			auto& ui = As<UIInvoke>(ev);

			// The command consists of a directive separated from its arguments by a space
			char directive[256];
			const char* p = ui.command;
			for (int i = 0; i < 256; ++i)
			{
				if (p[i] == 0 || p[i] == ' ')
				{
					directive[i] = 0;
					break;
				}
				else
				{
					directive[i] = p[i];
				}
			}

			directive[255] = 0;

			auto i = handlers.find(directive);
			if (i != handlers.end())
			{
				FN_OnCommand method = i->second.method;
				ICommandHandler* obj = i->second.handler;
				method(obj, ui.command);
			}
			else
			{
				Throw(0, "Unhandled UIInvoke handler for \"%s\". Review the REGISTER_UI_EVENT_HANDLER(...) sections in code", directive);
			}
		}
		else if (ev == evUIPopulate)
		{
			auto& pop = As<UIPopulate>(ev);

			if (pop.renderElement == nullptr)
			{
				auto i = renderElements.find(pop.name);
				if (i != renderElements.end())
				{
					pop.renderElement = i->second;
				}
				else
				{
					// Unhandled population event;
				}
			}
		}
	}

	void RegisterPopulator(cstr name, IUIElement* renderElement) override
	{
		renderElements[name] = renderElement;
	}

	void UnregisterPopulator(IUIElement* renderElement) override
	{
		auto i = renderElements.begin();
		while (i != renderElements.end())
		{
			if (i->second == renderElement)
			{
				i = renderElements.erase(i);
			}
			else
			{
				i++;
			}
		}
	}

	void RegisterEventHandler(ICommandHandler* handler, FN_OnCommand method, cstr cmd, cstr helpString) override
	{
		handlers[cmd] = { HString(helpString == nullptr ? "" : helpString), handler, method };
	}

	void UnregisterEventHandler(ICommandHandler* handler)
	{
		auto i = handlers.begin();
		while (i != handlers.end())
		{
			if (i->second.handler == handler)
			{
				i = handlers.erase(i);
			}
			else
			{
				i++;
			}
		}
	}

	enum { MessageHeightPixels = 32 };

	bool HasRoomForMessage(const GuiRect& logRect) const
	{
		if (scrollingMessages.empty())
		{
			return true;
		}

		auto& last = scrollingMessages.back();

		if (last.y + MessageHeightPixels > logRect.bottom)
		{
			return false;
		}

		return true;
	}

	Time::ticks lastScrollCheck = 0;
	int logAlpha = 0;

	double deltaScrollTime = 0;

	void ScrollMessages(const GuiRect& logRect)
	{
		Time::ticks now = Time::TickCount();

		if (scrollingMessages.empty())
		{
			lastScrollCheck = now;
			return;
		}
		
		Time::ticks dt = now - lastScrollCheck;

		double DT = clamp((double)dt / (double) Time::TickHz(), 0.0, 0.16);

		deltaScrollTime += DT;

		const double pixelsScrolledPerSecond = 60.0;
		
		double deltaScroll = pixelsScrolledPerSecond * deltaScrollTime;

		while (deltaScroll >= 1.0f)
		{
			deltaScroll -= 1.0f;
			deltaScrollTime -= 1.0 / pixelsScrolledPerSecond;

			lastScrollCheck = now;
			
			for (auto& m : scrollingMessages)
			{
				m.y -= 1;
			}
		}

		deltaScrollTime = clamp(deltaScrollTime, 0.0, 1.0);

		if (scrollingMessages.front().y < logRect.top)
		{
			scrollingMessages.erase(scrollingMessages.begin());
		}
	}

	void Render(IGuiRenderContext& grc) override
	{
		GuiMetrics metrics;
		grc.Renderer().GetGuiMetrics(metrics);

		if (metrics.screenSpan != lastSpan)
		{
			lastSpan = metrics.screenSpan;
			GuiRect screenRect{ 0, 0, lastSpan.x, lastSpan.y };

			for (auto& p : panels)
			{
				p.panel->SetRect(screenRect);
			}
		}

		if (lastSpan.x == 0 || lastSpan.y == 0) return;

		RenderPanels(grc);
		RenderScrollingLog(grc);
		RenderFileErrors(grc);
	}

	void RenderPanels(IGuiRenderContext& grc)
	{
		if (panels.empty())
		{
			return;
		}

		Modality modality;
		modality.isUnderModal = false;

		for (size_t i = 0; i < panels.size() - 1; ++i)
		{
			auto& p = panels[i];
			auto& rect = p.panel->ClientRect();
			modality.isModal = p.isModal;
			modality.isTop = false;
			modality.isUnderModal = false;

			for (size_t j = i + 1; j < panels.size(); ++j)
			{
				if (panels[j].isModal)
				{
					modality.isUnderModal = true;
					break;
				}
			}

			p.panel->Render(grc, { rect.left, rect.top }, modality);
		}

		auto& p = panels.back();
		auto& rect = p.panel->ClientRect();

		modality.isUnderModal = false;
		modality.isTop = true;
		modality.isModal = p.isModal;
		p.panel->Render(grc, { rect.left, rect.top }, modality);
	}

	void RenderFileErrors(IGuiRenderContext& grc)
	{
		if (mapFileNameToErrorMessages.empty())
		{
			return;
		}

		GuiMetrics metrics;
		grc.Renderer().GetGuiMetrics(metrics);

		GuiRect logRect;
		logRect.left = 2;
		logRect.right = metrics.screenSpan.x - 2;
		logRect.bottom = metrics.screenSpan.y - 1;
		logRect.top = logRect.top + 100;

		Graphics::DrawRectangle(grc, logRect, RGBAb(0, 0, 0, 255), RGBAb(64, 64, 64, 255));
		Graphics::DrawBorderAround(grc, Expand(logRect, 1), { 1,1 }, RGBAb(255, 192, 192, 255), RGBAb(255, 255, 255, 255));

		int y = logRect.top + MessageHeightPixels / 2;

		for (auto& i : mapFileNameToErrorMessages)
		{
			for (auto& m : i.second)
			{
				Graphics::RenderVerticalCentredText(grc, m, RGBAb(255, 255, 255), MessageHeightPixels, { 4, y }, &logRect);
				y += MessageHeightPixels;
			}
		}
	}

	void RenderScrollingLog(IGuiRenderContext& grc)
	{
		GuiMetrics metrics;
		grc.Renderer().GetGuiMetrics(metrics);

		if (scrollingMessages.empty() && messageLog.IsEmpty())
		{
			return;
		}

		GuiRect logRect;
		logRect.left = 2;
		logRect.right = metrics.screenSpan.x - 2;
		logRect.bottom = metrics.screenSpan.y - 1;
		logRect.top = logRect.bottom - 400;

		if (logAlpha)
		{
			Graphics::DrawRectangle(grc, logRect, RGBAb(0, 0, 0, (uint8)logAlpha), RGBAb(64, 64, 64, (uint8)logAlpha));
			Graphics::DrawBorderAround(grc, Expand(logRect, 1), { 1,1 }, RGBAb(192, 192, 192, (uint8)logAlpha), RGBAb(255, 255, 255, (uint8)logAlpha));
		}

		for (auto& m : scrollingMessages)
		{
			Graphics::RenderVerticalCentredText(grc, m.message.text, RGBAb(255, 255, 255), MessageHeightPixels, { 4,m.y }, &logRect);
		}

		if (!messageLog.IsEmpty() && HasRoomForMessage(logRect))
		{
			VisibleMessage top;
			if (messageLog.TryPopFront(top.message))
			{
				top.y = logRect.bottom - MessageHeightPixels;
				if (!scrollingMessages.empty())
				{
					top.y = scrollingMessages.back().y + MessageHeightPixels;
				}

				scrollingMessages.push_back(top);				
				logAlpha = 192;
			}
		}

		if (scrollingMessages.empty() && logAlpha > 0)
		{
			logAlpha--;
		}

		ScrollMessages(logRect);
	}

	Vec2i lastSpan = { 0,0 };

	void PushTop(IPaneSupervisor* panel, bool isModal) override
	{
		panels.push_back({ panel,isModal });

		GuiRect screenRect{ 0, 0, lastSpan.x, lastSpan.y };
		panel->SetRect(screenRect);
	}

	IPaneSupervisor* Pop() override
	{
		if (panels.empty()) Throw(0, "GuiStack: panels empty, nothing to pop");

		auto* p = panels.back().panel;
		panels.pop_back();
		return p;
	}

	IPaneSupervisor* Top() override
	{
		return panels.empty() ? nullptr : panels.back().panel;
	}

	IPaneBuilderSupervisor* BindPanelToScript(cstr scriptName, IScriptCompilationEventHandler* onCompile, IScriptEnumerator* implicitIncludes) override
	{
		return Rococo::MPlatImpl::CreateScriptedPanel(*platform, scriptName, onCompile, implicitIncludes);
	}

	IPaneBuilderSupervisor* GuiStack::CreateDebuggingOverlay()
	{
		return Rococo::MPlatImpl::CreateDebuggingOverlay(*platform);
	}
};

namespace Rococo
{
	namespace MPlatImpl
	{
		IGuiStackSupervisor* CreateGui(IPublisher& publisher, ISourceCache& cache, IRenderer& renderer, IUtilities& utils)
		{
			return new GuiStack(publisher, cache, renderer, utils);
		}
	}
}