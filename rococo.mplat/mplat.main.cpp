#include <rococo.mplat.h>
#include <rococo.os.win32.h>

#include <rococo.window.h>
#include <rococo.sexy.ide.h>
#include <rococo.dx11.renderer.win32.h>
#include <rococo.strings.h>

#include <vector>
#include <algorithm>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include <rococo.fonts.h>
#include <rococo.variable.editor.h>

#include <Commdlg.h>
#include <Processthreadsapi.h>
#include <psapi.h>

using namespace Rococo;
using namespace Rococo::Events;
using namespace Rococo::Windows;

EventId evFileUpdated = "OnFileUpdated"_event;
struct FileUpdatedEvent : public Event
{
   FileUpdatedEvent() : Event(evFileUpdated) {}
   cstr pingPath;
};

namespace Rococo
{
	namespace M
	{
		bool QueryYesNo(IWindow& ownerWindow, cstr message)
		{
			rchar title[256];
			GetWindowTextA(ownerWindow, title, 256);
			return ShowMessageBox(Windows::NullParent(), message, title, MB_ICONQUESTION | MB_YESNO) == IDYES;
		}

		void InitScriptSystem(IInstallation& installation);
	}
}


const char* CreatePersistentString(const char* volatileString)
{
   static std::unordered_set<std::string> persistentStrings;
   auto i = persistentStrings.find(volatileString);
   if (i == persistentStrings.end())
   {
      i = persistentStrings.insert(std::string(volatileString)).first;
   }

   return i->c_str();
}

EventId CreateEventIdFromVolatileString(const char* volatileString)
{
   auto* s = CreatePersistentString(volatileString);
   return EventId(s, (EventHash)FastHash(s));
}

namespace Rococo
{
	namespace M
	{
		void RunEnvironmentScript(Platform& platform, IEventCallback<ScriptCompileArgs>& _onScriptEvent, const char* name, bool addPlatform);
	}

	namespace Events
	{
		EventId BuysEventId = "mplat.busy"_event;
	}
}

class Utilities : public IUtilitiies
{
	IInstallation& installation;
	IRenderer& renderer;
public:
	Utilities(IInstallation& _installation, IRenderer& _renderer) : installation(_installation), renderer(_renderer) {}

	IScrollbar* CreateScrollbar(IKeyboardSupervisor& keyboard, bool _isVertical) override;

	void AddSubtitle(Platform& platform, cstr subtitle)
	{
		char fullTitle[256];

		if (subtitle && subtitle[0])
		{
			SafeFormat(fullTitle, sizeof(fullTitle), "%s - %s", platform.title, subtitle);
		}
		else
		{
			SafeFormat(fullTitle, sizeof(fullTitle), "%s", platform.title);
		}

		SetWindowTextA(platform.renderer.Window(), fullTitle);
	}

	bool GetLoadLocation(Windows::IWindow& parent, LoadDesc& ld) override
	{
		char filter[128];
		SecureFormat(filter, sizeof(filter), "%s%c%s%c%c", ld.extDesc, 0, ld.ext, 0, 0);

		OPENFILENAMEA dialog = { 0 };
		dialog.lStructSize = sizeof(dialog);
		dialog.hwndOwner = parent;
		dialog.lpstrFilter = filter;
		dialog.nFilterIndex = 1;
		dialog.lpstrFile = ld.path;
		dialog.nMaxFile = sizeof(ld.path);
		dialog.lpstrTitle = ld.caption;
		dialog.Flags = OFN_CREATEPROMPT | OFN_ENABLESIZING | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
		dialog.lpstrDefExt = ld.ext;

		renderer.SwitchToWindowMode();

		if (GetOpenFileNameA(&dialog))
		{
			ld.shortName = ld.path + dialog.nFileOffset;
			return true;
		}
		else
		{
			int error = GetLastError();
			if (error != 0) Throw(error, "Error GetOpenFileNameA");
			return false;
		}
	}

	bool GetSaveLocation(Windows::IWindow& parent, SaveDesc& sd) override
	{
		char filter[128];
		SecureFormat(filter, sizeof(filter), "%s%c%s%c%c", sd.extDesc, 0, sd.ext, 0, 0);

		OPENFILENAMEA dialog = { 0 };
		dialog.lStructSize = sizeof(dialog);
		dialog.hwndOwner = parent;
		dialog.lpstrFilter = filter;
		dialog.nFilterIndex = 1;
		dialog.lpstrFile = sd.path;
		dialog.nMaxFile = sizeof(sd.path);
		dialog.lpstrTitle = sd.caption;
		dialog.Flags = OFN_CREATEPROMPT | OFN_ENABLESIZING | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
		dialog.lpstrDefExt = sd.ext;

		renderer.SwitchToWindowMode();

		if (GetSaveFileNameA(&dialog))
		{
			sd.shortName = sd.path + dialog.nFileOffset;
			return true;
		}
		else
		{
			int error = GetLastError();
			if (error != 0) Throw(error, "Error GetSaveFileNameA");
			return false;
		}
	}

	void EnumerateFiles(IEventCallback<cstr>& cb, cstr pingPathDirectory) override
	{
		struct : IEventCallback<cstr>
		{
			std::vector<std::string> allResults;
			virtual void OnEvent(cstr filename)
			{
				allResults.push_back(filename);
			}
		} onFileFound;

		if (pingPathDirectory == nullptr || pingPathDirectory[0] != '!')
		{
			Throw(0, "Directories must be inside the content directory. Use the '!<directory>' notation");
		}

		rchar shortdir[IO::MAX_PATHLEN];
		rchar directory[IO::MAX_PATHLEN];

		StackStringBuilder sb(shortdir, _MAX_PATH);
		sb << pingPathDirectory;

		EndDirectoryWithSlash(shortdir, IO::MAX_PATHLEN);

		SafeFormat(directory, IO::MAX_PATHLEN, "%s%s", installation.Content(), (shortdir + 1));
		IO::ForEachFileInDirectory(directory, onFileFound);

		std::sort(onFileFound.allResults.begin(), onFileFound.allResults.end());

		for (auto& s : onFileFound.allResults)
		{
			rchar contentRelativePath[IO::MAX_PATHLEN];
			SafeFormat(contentRelativePath, IO::MAX_PATHLEN, "%s%s", shortdir, s.c_str());
			cb.OnEvent(contentRelativePath);
		}
	}

	bool QueryYesNo(Platform& platform, Windows::IWindow& parent, cstr question, cstr caption) override
	{
		cstr title = caption == nullptr ? platform.title : caption;
		renderer.SwitchToWindowMode();
		return ShowMessageBox(parent, question, title, MB_ICONQUESTION | MB_YESNO) == IDYES;
	}

	void RunEnvironmentScript(Platform& platform, IEventCallback<ScriptCompileArgs>& _onScriptEvent, const char* name, bool addPlatform) override
	{
		return M::RunEnvironmentScript(platform, _onScriptEvent, name, addPlatform);
	}

	void ShowErrorBox(Windows::IWindow& parent, IException& ex, cstr message) override
	{
		renderer.SwitchToWindowMode();
		OS::ShowErrorBox(parent, ex, message);
	}

	void RefreshResource(Platform& platform, cstr pingPath) override
	{
		FileUpdatedEvent fileUpdated;
		fileUpdated.pingPath = pingPath;

		platform.sourceCache.Release(pingPath);

		platform.publisher.Publish(fileUpdated);
	}

	IVariableEditor* CreateVariableEditor(Windows::IWindow& window, const Vec2i& span, int32 labelWidth, cstr appQueryName, cstr defaultTab, cstr defaultTooltip, IVariableEditorEventHandler* eventHandler, const Vec2i* topLeft) override
	{
		renderer.SwitchToWindowMode();
		return Rococo::CreateVariableEditor(window, span, labelWidth, appQueryName, defaultTab, defaultTooltip, eventHandler, topLeft);
	}

	virtual void SaveBinary(cstr pathname, const void* buffer, size_t nChars)
	{
		FileHandle fh = CreateFileA(pathname, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (fh == INVALID_HANDLE_VALUE)
		{
			Throw(GetLastError(), "Error saving %s", pathname);
		}

		if (!WriteFile(fh, buffer, (DWORD)nChars, nullptr, nullptr))
		{
			Throw(GetLastError(), "Error writing %s", pathname);
		}
	}
};

class GuiStack : public IGUIStack, public IObserver
{
	struct PanelDesc
	{
		IPanelSupervisor* panel;
		bool isModal;
	};
	std::vector<PanelDesc> panels;

	struct CommandHandler
	{
		std::string helpString;
		ICommandHandler* handler;
		FN_OnCommand method;
	};

	IPublisher& publisher;
	ISourceCache& sourceCache;
	IRenderer& renderer;
	IUtilitiies& utilities;

	std::unordered_map<std::string, CommandHandler> handlers;
	std::unordered_map<std::string, IUIElement*> renderElements;
public:
	Platform* platform;

	GuiStack(IPublisher& _publisher, ISourceCache& _sourceCache, IRenderer& _renderer, IUtilitiies& _utilities) :
		publisher(_publisher),
		sourceCache(_sourceCache),
		renderer(_renderer),
		utilities(_utilities)
	{
		publisher.Attach(this, UIInvoke::EvId());
		publisher.Attach(this, UIPopulate::EvId());
	}

	~GuiStack()
	{
		publisher.Detach(this);
	}

	void AppendEvent(const MouseEvent& me) override
	{
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

	bool AppendEvent(const KeyboardEvent& ke)
	{
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
		if (ev.id == UIInvoke::EvId())
		{
			auto& ui = As<UIInvoke>(ev);

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
		}
		else if (ev.id == UIPopulate::EvId())
		{
			auto& pop = As<UIPopulate>(ev);

			auto i = renderElements.find(pop.name);
			if (i != renderElements.end())
			{
				pop.renderElement = i->second;
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
		handlers[cmd] = { std::string(helpString == nullptr ? "" : helpString), handler, method };
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

	void Render(IGuiRenderContext& grc) override
	{
		if (panels.empty()) return;

		bool hiddenByModal = false;

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

	void PushTop(IPanelSupervisor* panel, bool isModal) override
	{
		panels.push_back({ panel,isModal });
	}

	IPanelSupervisor* Pop() override
	{
		if (panels.empty()) Throw(0, "GuiStack: panels empty, nothing to pop");

		auto* p = panels.back().panel;
		panels.pop_back();
		return p;
	}

	IPanelSupervisor* Top() override
	{
		return panels.empty() ? nullptr : panels.back().panel;
	}

	IPaneBuilderSupervisor* BindPanelToScript(cstr scriptName) override;

	IPaneBuilderSupervisor* CreateOverlay() override;
};

class BasePanel : public IPanelSupervisor
{
	GuiRect rect{ 0, 0, 0, 0 };
	bool isVisible{ true };

	std::vector<IPanelSupervisor*> children;

	struct UICommand
	{
		std::string command;
		boolean32 defer;
	};

	std::vector<UICommand> commands;

	struct UIPopulator
	{
		std::string name;
	};

	std::vector<UIPopulator> populators;

	ColourScheme scheme
	{
	   RGBAb(160,160,160, 192),
	   RGBAb(192,192,192, 192),
	   RGBAb(255,255,255, 224),
	   RGBAb(224,224,224, 224),
	   RGBAb(255,255,255, 255),
	   RGBAb(160,160,160, 224),
	   RGBAb(192,192,192, 224),
	   RGBAb(255,255,255, 255),
	   RGBAb(224,224,224, 255),
	   RGBAb(255,255,255, 255),
	};

public:
	void SetCommand(int32 stateIndex, boolean32 deferAction, const fstring& text) override
	{
		if (stateIndex < 0 || stateIndex > 4)
		{
			Throw(0, "BasePanel::SetCommand: stateIndex %d out of bounds [0,4]", stateIndex);
		}

		if (stateIndex >= commands.size())
		{
			commands.resize(stateIndex + 1);
		}

		commands[stateIndex] = { std::string(text), deferAction };
	}

	void Invoke(IPublisher& publisher, int32 stateIndex)
	{
		if (stateIndex >= 0 && stateIndex < (int32)commands.size())
		{
			auto& c = commands[stateIndex];

			if (!c.command.empty())
			{
				UIInvoke invoke;
				SecureFormat(invoke.command, sizeof(invoke.command), "%s", c.command.c_str());

				if (c.defer)
				{
					publisher.Post(invoke, true);
				}
				else
				{
					publisher.Publish(invoke);
				}
			}
		}
	}

	void SetPopulator(int32 stateIndex, const fstring& populatorName) override
	{
		if (stateIndex < 0 || stateIndex > 4)
		{
			Throw(0, "BasePanel::SetCommand: stateIndex %d out of bounds [0,4]", stateIndex);
		}

		if (stateIndex >= populators.size())
		{
			populators.resize(stateIndex + 1);
		}

		if (populatorName.length >= 192)
		{
			Throw(0, "BasePanel::SetPopulator(...): Maximum length for populator name is 192 chars");
		}

		populators[stateIndex] = { std::string(populatorName) };
	}

	void AppendEventToChildren(IPublisher& publisher, const MouseEvent& me, const Vec2i& absTopLeft, int stateIndex = 0)
	{
		int32 hitCount = 0;

		for (auto i : children)
		{
			auto& rect = i->ClientRect();
			auto topLeft = TopLeft(i->ClientRect()) + absTopLeft;
			auto span = Span(rect);
			GuiRect childRect{ topLeft.x, topLeft.y, topLeft.x + span.x, topLeft.y + span.y };
			if (IsPointInRect(me.cursorPos, childRect))
			{
				i->AppendEvent(me, topLeft);
				hitCount++;
			}
		}

		if (hitCount == 0 && stateIndex >= 0 && stateIndex < (int32)populators.size())
		{
			if (!populators[stateIndex].name.empty())
			{
				UIPopulate populate;
				populate.renderElement = nullptr;
				populate.name = populators[stateIndex].name.c_str();
				publisher.Publish(populate);

				if (populate.renderElement)
				{
					populate.renderElement->OnRawMouseEvent(me);

					if (me.HasFlag(MouseEvent::LUp) || me.HasFlag(MouseEvent::LDown))
					{
						populate.renderElement->OnMouseLClick(me.cursorPos, me.HasFlag(MouseEvent::LDown));
					}
					else if (me.HasFlag(MouseEvent::RUp) || me.HasFlag(MouseEvent::RDown))
					{
						populate.renderElement->OnMouseRClick(me.cursorPos, me.HasFlag(MouseEvent::RDown));
					}
					else
					{
						int dz = ((int32)(short)me.buttonData) / 120;
						populate.renderElement->OnMouseMove(me.cursorPos, { me.dx, me.dy }, dz);
					}
				}
			}
		}
	}

	bool AppendEventToChildren(IPublisher& publisher, const KeyboardEvent& ke, const Vec2i& focusPoint, const Vec2i& absTopLeft, int stateIndex = 0)
	{
		for (auto i : children)
		{
			auto& rect = i->ClientRect();
			auto topLeft = TopLeft(i->ClientRect()) + absTopLeft;
			auto span = Span(rect);
			GuiRect childRect{ topLeft.x, topLeft.y, topLeft.x + span.x, topLeft.y + span.y };
			if (IsPointInRect(focusPoint, childRect))
			{
				if (i->AppendEvent(ke, focusPoint, topLeft))
				{
					return true;
				}
			}
		}

		if (stateIndex >= 0 && stateIndex < (int32)populators.size())
		{
			if (!populators[stateIndex].name.empty())
			{
				UIPopulate populate;
				populate.renderElement = nullptr;
				populate.name = populators[stateIndex].name.c_str();
				publisher.Publish(populate);

				if (populate.renderElement)
				{
					if (populate.renderElement->OnKeyboardEvent(ke))
					{
						return true;
					}
				}
			}
		}

		return false;
	}

	void AlignLeftEdges(int32 x, boolean32 preserveSpan) override
	{
		for (auto i : children)
		{
			GuiRect rect = i->ClientRect();
			if (preserveSpan)
			{
				Vec2i span = Span(rect);
				rect.right = x + span.x;
			}

			rect.left = x;
			i->SetRect(rect);
		}
	}

	void AlignRightEdges(int32 x, boolean32 preserveSpan) override
	{
		int x0 = (rect.right - rect.left) - x;
		for (auto i : children)
		{
			GuiRect rect = i->ClientRect();
			if (preserveSpan)
			{
				Vec2i span = Span(rect);
				rect.left = x - span.x;
			}

			rect.right = x0;
			i->SetRect(rect);
		}
	}

	void LayoutVertically(int32 vertBorder, int32 vertSpacing) override
	{
		if (children.empty()) return;

		if (vertSpacing < 0)
		{
			// Vertically centred

			int dy = 0;
			for (auto i : children)
			{
				GuiRect rect = i->ClientRect();
				Vec2i span = Span(rect);
				dy += span.y;
			}

			int containerHeight = rect.bottom - rect.top - 2 * vertBorder;
			int freeSpace = containerHeight - dy;
			vertSpacing = freeSpace / (int32)children.size();
		}

		int y = vertBorder;

		if (vertBorder < 0)
		{
			int dy = 0;
			for (auto i : children)
			{
				GuiRect rect = i->ClientRect();
				Vec2i span = Span(rect);
				dy += span.y + vertSpacing;
			}

			y = rect.bottom + vertBorder - dy;
		}

		for (auto i : children)
		{
			GuiRect rect = i->ClientRect();
			Vec2i span = Span(rect);
			rect.top = y;
			rect.bottom = y + span.y;
			i->SetRect(rect);

			y = rect.bottom + vertSpacing;
		}
	}


	void SetRect(const GuiRect& rect) override
	{
		this->rect = rect;
	}

	const GuiRect& ClientRect() const override
	{
		return rect;
	}

	void GetRect(GuiRect& outRect) override
	{
		outRect = rect;
	}

	boolean32 IsVisible() override
	{
		return isVisible;
	}

	boolean32 IsNormalized() override
	{
		return rect.right > rect.left && rect.bottom > rect.top;
	}

	void SetVisible(boolean32 visible) override
	{
		this->isVisible = isVisible;
	}

	IPanelSupervisor* operator[](int index) override
	{
		return GetChild(index);
	}

	IPanelSupervisor* GetChild(int index)
	{
		if (index < 0 || index >= children.size())
		{
			Throw(0, "BasePanel[index] -> index %d was out of bounds. Child count: %d", index, children.size());
		}

		return children[index];
	}

	int Children() const override
	{
		return (int)children.size();
	}

	void AddChild(IPanelSupervisor* child) override
	{
		children.push_back(child);
	}

	void RemoveChild(IPanelSupervisor* child) override
	{
		auto i = std::remove(children.begin(), children.end(), child);
		children.erase(i, children.end());
	}

	void FreeAllChildren() override
	{
		for (auto i : children)
		{
			i->FreeAllChildren();
			i->Free();
		}
		children.clear();
	}

	void SetScheme(const ColourScheme& scheme) override
	{
		this->scheme = scheme;
	}

	const ColourScheme& Scheme() const
	{
		return scheme;
	}

	void SetColourBk1(RGBAb normal, RGBAb hilight) override
	{
		scheme.topLeft = normal;
		scheme.hi_topLeft = hilight;
	}

	void SetColourBk2(RGBAb normal, RGBAb hilight) override
	{
		scheme.bottomRight = normal;
		scheme.hi_bottomRight = hilight;
	}

	void SetColourEdge1(RGBAb normal, RGBAb hilight) override
	{
		scheme.topLeftEdge = normal;
		scheme.hi_topLeftEdge = hilight;
	}

	void SetColourEdge2(RGBAb normal, RGBAb hilight) override
	{
		scheme.bottomRightEdge = normal;
		scheme.hi_bottomRightEdge = hilight;
	}

	void SetColourFont(RGBAb normal, RGBAb hilight) override
	{
		scheme.fontColour = normal;
		scheme.hi_fontColour = hilight;
	}

	void Populate(IPublisher& publisher, IGuiRenderContext& grc, int32 stateIndex, const Vec2i& topLeft)
	{
		if (stateIndex >= 0 && stateIndex < (int32)populators.size())
		{
			UIPopulate populate;
			populate.renderElement = nullptr;
			populate.name = populators[stateIndex].name.c_str();
			publisher.Publish(populate);

			if (populate.renderElement)
			{
				GuiRect absRect = GuiRect{ 0, 0, Width(rect), Height(rect) } +topLeft;
				populate.renderElement->Render(grc, absRect);
			}
		}
	}

	void RenderBackground(IGuiRenderContext& grc, const Vec2i& topLeft, const Modality& modality)
	{
		GuiMetrics metrics;
		grc.Renderer().GetGuiMetrics(metrics);

		auto p = metrics.cursorPosition;

		GuiRect absRect{ topLeft.x, topLeft.y, topLeft.x + rect.right - rect.left, topLeft.y + rect.bottom - rect.top };

		if (!modality.isUnderModal && IsPointInRect(p, absRect))
		{
			Graphics::DrawRectangle(grc, absRect, scheme.hi_topLeft, scheme.hi_bottomRight);
			Graphics::DrawBorderAround(grc, absRect, Vec2i{ 1, 1 }, scheme.hi_topLeftEdge, scheme.hi_bottomRightEdge);
		}
		else
		{
			Graphics::DrawRectangle(grc, absRect, scheme.topLeft, scheme.bottomRight);
			Graphics::DrawBorderAround(grc, absRect, Vec2i{ 1, 1 }, scheme.topLeftEdge, scheme.bottomRightEdge);
		}
	}

	void RenderChildren(IGuiRenderContext& grc, const Vec2i& topLeft, const Modality& modality)
	{
		auto& currentRect = ClientRect();
		if (IsVisible())
		{
			if (IsNormalized()) RenderBackground(grc, topLeft, modality);

			for (auto i : children)
			{
				auto childRect = i->ClientRect() + topLeft;
				i->Render(grc, { childRect.left, childRect.top }, modality);
			}
		}
	}
};

bool operator != (const GuiRect& a, const GuiRect& b)
{
   return a.left != b.left || a.right != b.right || a.bottom != b.bottom || a.top != b.top;
}

class PanelContainer : public BasePanel, public IPaneContainer
{
	Platform& platform;
public:
	PanelContainer(Platform& _platform) : platform(_platform)
	{

	}

	Rococo::IPaneContainer* AddContainer(const GuiRect& rect)
	{
		auto* container = new PanelContainer(platform);
		AddChild(container);
		container->SetRect(rect);
		return container;
	}

	Rococo::ITabContainer* AddTabContainer(int32 tabHeight, int32 fontIndex, const GuiRect& rect) override;
	Rococo::ITextOutputPane* AddTextOutput(int32 fontIndex, const fstring& eventKey, const GuiRect& rect) override;
	Rococo::ILabelPane* AddLabel(int32 fontIndex, const fstring& text, const GuiRect& rect) override;
	Rococo::ISlider* AddSlider(int32 fontIndex, const fstring& text, const GuiRect& rect, float minValue, float maxValue) override;
	Rococo::IRadioButton* AddRadioButton(int32 fontIndex, const fstring& text, const fstring& key, const fstring& value, const GuiRect& rect) override;
	Rococo::IScroller* AddScroller(const fstring& key, const GuiRect& rect, boolean32 isVertical) override;

	IPane* Base() override
	{
		return this;
	}

	void Free() override
	{
		FreeAllChildren();
		delete this;
	}

	bool AppendEvent(const KeyboardEvent& ke, const Vec2i& focusPoint, const Vec2i& absTopLeft)
	{
		return AppendEventToChildren(platform.publisher, ke, focusPoint, absTopLeft, 0);
	}

	void AppendEvent(const MouseEvent& me, const Vec2i& absTopLeft)
	{
		AppendEventToChildren(platform.publisher, me, absTopLeft, 0);
	}

	void Render(IGuiRenderContext& grc, const Vec2i& topLeft, const Modality& modality) override
	{
		Populate(platform.publisher, grc, 0, topLeft);
		RenderChildren(grc, topLeft, modality);
	}
};

void RenderLabel(IGuiRenderContext& grc, cstr text, const GuiRect& absRect, int horzAlign, int vertAlign, Vec2i padding, int fontIndex, const ColourScheme& scheme, bool enableHighlights)
{
	GuiMetrics metrics;
	grc.Renderer().GetGuiMetrics(metrics);

	using namespace Rococo::Fonts;
	struct : IDrawTextJob
	{
		cstr text;
		int fontIndex;
		RGBAb colour;

		virtual void OnDraw(IGlyphBuilder& builder)
		{
			builder.SetTextColour((FontColour&)colour);
			builder.SetFontIndex(fontIndex);

			for (cstr p = text; *p != 0; p++)
			{
				char c = *p;
				GuiRectf outputRect;
				builder.AppendChar(c, outputRect);
			}
		}
	} job;

	job.text = text;
	job.fontIndex = fontIndex;
	job.colour = enableHighlights && IsPointInRect(metrics.cursorPosition, absRect) ? scheme.hi_fontColour : scheme.fontColour;

	Vec2i span = grc.EvalSpan({ 0,0 }, job, nullptr);

	Vec2i pos;

	if (horzAlign < 0)
	{
		pos.x = absRect.left + padding.x;
	}
	else if (horzAlign == 0)
	{
		pos.x = Centre(absRect).x - (span.x >> 1);
	}
	else
	{
		pos.x = absRect.right - span.x - padding.x;
	}

	if (vertAlign < 0)
	{
		pos.y = absRect.top + padding.y;
	}
	else if (vertAlign == 0)
	{
		pos.y = Centre(absRect).y - (span.y >> 1);
	}
	else
	{
		pos.y = absRect.bottom - span.y - padding.y;
	}

	GuiRect clipRect = absRect;
	clipRect.left += (padding.x - 1);
	clipRect.right -= (padding.x - 1);
	clipRect.top += (padding.y - 1);
	clipRect.bottom -= (padding.y + 1);

	grc.RenderText(pos, job, &clipRect);
}

class PanelRadioButton : public BasePanel, public IRadioButton, public IObserver
{
	int32 fontIndex = 1;
	rchar text[128];
	EventId id;
	rchar value[128];
	int32 horzAlign = 0;
	int32 vertAlign = 0;
	Vec2i padding{ 0,0 };
	IPublisher& publisher;

	int32 stateIndex = -1; // indeterminate
public:
	PanelRadioButton(IPublisher& _publisher, int _fontIndex, cstr _text, cstr _key, cstr _value) :
		id(CreateEventIdFromVolatileString(_key)),
		fontIndex(_fontIndex), publisher(_publisher)
	{
		SafeFormat(text, sizeof(text), "%s", _text);
		SafeFormat(value, sizeof(value), "%s", _value);

		publisher.Attach(this, id);
	}

	~PanelRadioButton()
	{
		publisher.Detach(this);
	}

	void OnEvent(Event& ev) override
	{
		if (ev.id == id)
		{
			auto& toe = As<TextOutputEvent>(ev);
			if (!toe.isGetting)
			{
				stateIndex = (toe.text && Eq(toe.text, value)) ? 1 : 0;
			}
		}
	}

	void Free() override
	{
		delete this;
	}

	IPane* Base() override
	{
		return this;
	}

	void SetAlignment(int32 horz, int32 vert, int32 paddingX, int paddingY)
	{
		horzAlign = horz;
		vertAlign = vert;
		padding = { paddingX, paddingY };
	}

	bool AppendEvent(const KeyboardEvent& me, const Vec2i& focusPoint, const Vec2i& absTopLeft) override
	{
		return false;
	}

	void AppendEvent(const MouseEvent& me, const Vec2i& absTopLeft) override
	{
		if (stateIndex == 0 && me.HasFlag(me.LUp) || me.HasFlag(me.RUp))
		{
			TextOutputEvent toe(id);
			toe.isGetting = false;
			SecureFormat(toe.text, sizeof(toe.text), value);
			publisher.Publish(toe);
		}
	}

	void Render(IGuiRenderContext& grc, const Vec2i& topLeft, const Modality& modality) override
	{
		GuiMetrics metrics;
		grc.Renderer().GetGuiMetrics(metrics);

		if (stateIndex == -1)
		{
			TextOutputEvent toe(id);
			toe.isGetting = true;
			publisher.Publish(toe);
			stateIndex = (toe.text && Eq(toe.text, value)) ? 1 : 0;
		}

		auto p = metrics.cursorPosition;

		GuiRect absRect = GuiRect{ 0, 0, Width(ClientRect()), Height(ClientRect()) } +topLeft;
		auto& scheme = Scheme();

		if (stateIndex != 0)
		{
			Graphics::DrawRectangle(grc, absRect, scheme.hi_topLeft, scheme.hi_bottomRight);
			Graphics::DrawBorderAround(grc, absRect, Vec2i{ 1, 1 }, scheme.hi_topLeftEdge, scheme.hi_bottomRightEdge);
		}
		else
		{
			Graphics::DrawRectangle(grc, absRect, scheme.topLeft, scheme.bottomRight);
			Graphics::DrawBorderAround(grc, absRect, Vec2i{ 1, 1 }, scheme.topLeftEdge, scheme.bottomRightEdge);
		}

		RenderLabel(grc, text, absRect, horzAlign, vertAlign, padding, fontIndex, scheme, !modality.isUnderModal);
	}
};

class PanelLabel : public BasePanel, public ILabelPane
{
	int32 fontIndex = 1;
	rchar text[128];
	int32 horzAlign = 0;
	int32 vertAlign = 0;
	Vec2i padding{ 0,0 };
	IPublisher& publisher;
public:
	PanelLabel(IPublisher& _publisher, int _fontIndex, cstr _text) : fontIndex(_fontIndex), publisher(_publisher)
	{
		SafeFormat(text, sizeof(text), "%s", _text);
	}

	void Free() override
	{
		delete this;
	}

	IPane* Base() override
	{
		return this;
	}

	void SetAlignment(int32 horz, int32 vert, int32 paddingX, int paddingY)
	{
		horzAlign = horz;
		vertAlign = vert;
		padding = { paddingX, paddingY };
	}

	void AppendEvent(const MouseEvent& me, const Vec2i& absTopLeft) override
	{
		if (me.HasFlag(me.LUp) || me.HasFlag(me.RUp))
		{
			BasePanel::Invoke(publisher, 1);
		}
		else if (me.HasFlag(me.LDown) || me.HasFlag(me.RDown))
		{
			BasePanel::Invoke(publisher, 0);
		}
	}

	bool AppendEvent(const KeyboardEvent& me, const Vec2i& focusPoint, const Vec2i& absTopLeft) override
	{
		return false;
	}

	void Render(IGuiRenderContext& grc, const Vec2i& topLeft, const Modality& modality) override
	{
		RenderBackground(grc, topLeft, modality);

		auto span = Span(ClientRect());
		GuiRect absRect{ topLeft.x, topLeft.y, topLeft.x + span.x, topLeft.y + span.y };

		RenderLabel(grc, text, absRect, horzAlign, vertAlign, padding, fontIndex, BasePanel::Scheme(), !modality.isUnderModal);
	}
};

class PanelSlider : public BasePanel, public ISlider
{
	int32 fontIndex = 1;
	rchar text[128];
	IPublisher& publisher;
	IRenderer& renderer;
	float minValue;
	float maxValue;
	float value;
public:
	PanelSlider(IPublisher& _publisher, IRenderer& _renderer, int _fontIndex, cstr _text, float _minValue, float _maxValue) :
		publisher(_publisher), renderer(_renderer), fontIndex(_fontIndex), minValue(_minValue), maxValue(_maxValue)
	{
		SafeFormat(text, sizeof(text), "%s", _text);
		value = 0.5f * (maxValue + minValue);
	}

	void Free() override
	{
		delete this;
	}

	IPane* Base() override
	{
		return this;
	}

	bool AppendEvent(const KeyboardEvent& me, const Vec2i& focusPoint, const Vec2i& absTopLeft) override
	{
		return false;
	}

	void AppendEvent(const MouseEvent& me, const Vec2i& absTopLeft) override
	{
		if (me.HasFlag(me.LUp))
		{
			GuiMetrics metrics;
			renderer.GetGuiMetrics(metrics);

			auto ds = metrics.cursorPosition - absTopLeft;

			int32 width = Width(ClientRect());

			float delta = ds.x / (float)width;

			value = delta * (maxValue - minValue);
		}
	}

	void Render(IGuiRenderContext& grc, const Vec2i& topLeft, const Modality& modality) override
	{
		auto& controlRect = ClientRect();

		float fspan = maxValue - minValue;
		float delta = (value - minValue) / fspan;

		int32 right = (int32)(delta * (controlRect.right - controlRect.left));

		GuiRect sliderRect
		{
		   topLeft.x,
		   topLeft.y,
		   topLeft.x + right,
		   topLeft.y + Height(controlRect)
		};

		auto span = Span(ClientRect());
		GuiRect absRect{ topLeft.x, topLeft.y, topLeft.x + span.x, topLeft.y + span.y };

		GuiMetrics metrics;
		grc.Renderer().GetGuiMetrics(metrics);

		bool lit = !modality.isUnderModal && IsPointInRect(metrics.cursorPosition, absRect);
		Graphics::DrawRectangle(grc, sliderRect, lit ? Scheme().hi_topLeft : Scheme().topLeft, lit ? Scheme().hi_bottomRight : Scheme().bottomRight);

		char fullText[256];
		SafeFormat(fullText, sizeof(fullText), "%s - %0.0f%%", text, delta * 100.0f);

		RenderLabel(grc, fullText, absRect, 0, 0, { 0,0 }, fontIndex, BasePanel::Scheme(), !modality.isUnderModal);

		Graphics::DrawBorderAround(grc, absRect, { 1,1 }, lit ? Scheme().hi_topLeftEdge : Scheme().topLeftEdge, lit ? Scheme().hi_bottomRightEdge : Scheme().bottomRightEdge);
	}
};

struct Tab
{
	std::string caption;
	std::string panelText;
	int32 width;
	GuiRect lastRect{ 0,0,0,0 };
};

class TabContainer : public BasePanel, public ITabContainer
{
	int32 fontIndex;
	int32 tabHeight;
	IPublisher& publisher;
	IKeyboardSupervisor& keyboard;
	std::vector<Tab> tabs;
	size_t tabSelect = 0;
public:
	TabContainer(IPublisher& _publisher, IKeyboardSupervisor& _keyboard, int _tabHeight, int _fontIndex) :
		publisher(_publisher), keyboard(_keyboard), tabHeight(_tabHeight), fontIndex(_fontIndex)
	{
	}

	void Free() override
	{
		delete this;
	}

	IPane* Base() override
	{
		return this;
	}

	void AddTab(int32 width, const fstring& caption, const fstring& panelText) override
	{
		Tab tab{ caption, panelText, width };
		tabs.push_back(tab);
	}

	bool AppendEvent(const KeyboardEvent& k, const Vec2i& focusPoint, const Vec2i& absTopLeft) override
	{
		Key key = keyboard.GetKeyFromEvent(k);
		if (key.isPressed)
		{
			bool consume = true;

			if (Eq(key.KeyName, "HOME"))
			{
				tabSelect = 0;
			}
			else if (Eq(key.KeyName, "END"))
			{
				tabSelect = tabs.size() - 1;
			}
			else if (Eq(key.KeyName, "LEFT"))
			{
				tabSelect--;
			}
			else if (Eq(key.KeyName, "RIGHT"))
			{
				tabSelect++;
				if (tabSelect >= tabs.size()) tabSelect = tabs.size() - 1;
			}
			else if (Eq(key.KeyName, "TAB"))
			{
				tabSelect++;
			}
			else
			{
				consume = false;
			}

			if (tabSelect >= tabs.size()) tabSelect = 0;
		
			if (consume) return true;
		}

		if (tabSelect < tabs.size())
		{
			UIPopulate populate;
			populate.renderElement = nullptr;
			populate.name = tabs[tabSelect].panelText.c_str();
			publisher.Publish(populate);

			if (populate.renderElement)
			{
				return populate.renderElement->OnKeyboardEvent(k);
			}
		}

		return false;
	}

	void AppendEvent(const MouseEvent& me, const Vec2i& absTopLeft) override
	{
		if (me.HasFlag(MouseEvent::LUp))
		{
			for (size_t i = 0; i < tabs.size(); ++i)
			{
				if (IsPointInRect(me.cursorPos, tabs[i].lastRect))
				{
					tabSelect = i;
					return;
				}
			}

			if (IsPointInRect(me.cursorPos, leftButtonRect))
			{
				startIndex--;
				if (startIndex < 0) startIndex = 0;
				tabSelect--;
				if (tabSelect < 0) tabSelect = 0;
				return;
			}
			else if (IsPointInRect(me.cursorPos, rightButtonRect))
			{
				startIndex++;
				tabSelect++;

				if (tabSelect >= tabs.size())
				{
					tabSelect = tabs.size() - 1;
					if (tabSelect == -1) tabSelect = 0;
				}

				if (startIndex >= (int32)tabs.size())
				{
					startIndex = 0;
				}

				if (startIndex > tabSelect)
				{
					tabSelect = startIndex;
				}
				return;
			}
		}

		if (tabSelect < tabs.size())
		{
			UIPopulate populate;
			populate.renderElement = nullptr;
			populate.name = tabs[tabSelect].panelText.c_str();
			publisher.Publish(populate);

			if (populate.renderElement)
			{
				return populate.renderElement->OnRawMouseEvent(me);
			}
		}
	}

	void RenderTabButton(IGuiRenderContext& grc, const Tab& tab, bool isHilighted, size_t index)
	{
		RGBAb fontColour;
		if (index == tabSelect)
		{
			fontColour = RGBAb(0, 0, 0, 255);

			GuiRect selectedTabRect = tab.lastRect;
			selectedTabRect.bottom += 4;
			Graphics::DrawRectangle(grc, selectedTabRect, RGBAb(192, 192, 255, 255), RGBAb(224, 224, 255, 255));
		}
		else
		{
			fontColour = RGBAb(255, 255, 255, 255);
			Graphics::DrawLine(grc, 1, TopRight(tab.lastRect), BottomRight(tab.lastRect), RGBAb(128, 128, 128, 255));
		}

		if (isHilighted)
		{
			fontColour.alpha = 255;
		}
		else
		{
			fontColour.alpha = 224;
		}

		Vec2i middleLeft = { tab.lastRect.left + 6, 1 + ( (tab.lastRect.top + tab.lastRect.bottom) >> 1) };
		Graphics::RenderVerticalCentredText(grc, tab.caption.c_str(), fontColour, fontIndex, middleLeft);
	}

	int32 startIndex = 0; // The first tab index rendered on the tab control row
	int32 endIndex = 0; // The end iterator for iterating through tabs
	GuiRect leftButtonRect = { -1,-1,-1,-1 };
	GuiRect rightButtonRect = { -1,-1,-1,-1 };

	void DrawTriangleFacingLeft(IGuiRenderContext& grc, const GuiRect& container, RGBAb colour)
	{
		BaseVertexData noFont{ {0, 0}, 0 };
		SpriteVertexData solid{ 1.0f, 0, 0, 0 };

		GuiVertex triangle[3] =
		{
			{
				{ (float)container.left, (float)((container.top + container.bottom) >> 1) } ,
				noFont,
				solid,
				colour
			},
			{
				{ (float)container.right, (float)container.top },
				noFont,
				solid,
				colour
			},
			{
				{ (float)container.right, (float)container.bottom },
				noFont,
				solid,
				colour
			}
		};
		grc.AddTriangle(triangle);
	}

	void DrawTriangleFacingRight(IGuiRenderContext& grc, const GuiRect& container, RGBAb colour)
	{
		BaseVertexData noFont{ { 0,0 }, 0 };
		SpriteVertexData solidColour{ 1.0f, 0.0f, 0.0f, 0.0f };

		GuiVertex triangle[3] =
		{
			{
				{ (float)container.left, (float)container.top },
				noFont,
				solidColour,
				colour
			},
			{
				{ (float)container.left, (float)container.bottom },
				noFont,
				solidColour,
				colour
			},
			{
				{ (float)container.right, (float)((container.top + container.bottom) >> 1) },
				noFont,
				solidColour,
				colour
			}
		};
		grc.AddTriangle(triangle);
	}

	int32 DetermineFinalTabIndex(int32 startingFrom, int32 left, int32 right, int32 buttonWidth) const
	{
		int32 finalIndex = (int32)tabs.size();

		int32 x = left;

		for (int32 i = startingFrom; i < finalIndex; ++i)
		{
			auto& t = tabs[i];

			int32 rhs = x + t.width;
			if (rhs >= right)
			{
				// We don't have enough width to display all the tabs
				// Strip tabs until we have enough spaace to display tab buttons
				while (x + buttonWidth > right)
				{
					i--;

					if (i < startIndex)
					{
						return -1;
					}

					x -= tabs[i].width;
				}

				return i;
			}

			x = rhs;
		}

		return finalIndex;
	}

	void RenderControls(IGuiRenderContext& grc, const Vec2i& topLeft)
	{
		GuiRect rect;
		GetRect(rect);

		GuiRect absRect = GuiRect{ 0, 0, Width(rect), Height(rect) } +topLeft;

		int dy = tabHeight;

		GuiRect controlRect{ absRect.left + 1, topLeft.y + 1, absRect.right - 2, topLeft.y + tabHeight};
		Graphics::DrawRectangle(grc, controlRect, RGBAb(0, 0, 96, 255), RGBAb(0, 0, 128, 255));

		GuiMetrics metrics;
		grc.Renderer().GetGuiMetrics(metrics);

		int x = absRect.left;

		int32 availableWidth = Width(absRect);

		int32 tabButtonWidth = 20;

		if (tabSelect != -1 && (startIndex > (int32)tabSelect))
		{
			startIndex = (int32)tabSelect;
		}

		if (startIndex > 0 && startIndex < (int32) tabs.size())
		{
			leftButtonRect = GuiRect{ controlRect.left + 6, controlRect.top + 3, controlRect.left -6 + tabButtonWidth, controlRect.bottom - 3 };
			x = controlRect.left + tabButtonWidth;
		}
		else
		{
			leftButtonRect = GuiRect{ -1,-1,-1,-1 };
			x = absRect.left;
		}

		endIndex = DetermineFinalTabIndex(startIndex, x, absRect.right, tabButtonWidth);

		if (tabSelect != -1 && tabSelect >= endIndex)
		{
			for (int32 i = 0; i < (int32)tabs.size(); ++i)
			{
				int jEnd = DetermineFinalTabIndex(i, x, absRect.right, tabButtonWidth);
				if (jEnd > tabSelect)
				{
					startIndex = i;
					endIndex = jEnd;
					break;
				}
			}
		}

		for (auto& t : tabs)
		{
			t.lastRect = { -1,-1,-1,-1 };
		}

		for (size_t i = startIndex; i < endIndex; ++i)
		{
			auto& t = tabs[i];

			t.lastRect = GuiRect{ x, absRect.top + 1, x + t.width,absRect.top + tabHeight - 2 };

			RenderTabButton(grc, t, IsPointInRect(metrics.cursorPosition, t.lastRect), i);

			x += Width(t.lastRect);
		}

		DrawTriangleFacingLeft(grc, leftButtonRect, RGBAb(255, 255, 255));

		if (endIndex < tabs.size())
		{
			rightButtonRect = GuiRect{ absRect.right + 6 - tabButtonWidth, controlRect.top + 3, absRect.right - 6, controlRect.bottom - 3 };
			DrawTriangleFacingRight(grc, rightButtonRect, RGBAb(255, 255, 255));
		}
		else
		{
			rightButtonRect = { -1,-1,-1,-1 };
		}
	}

	void Render(IGuiRenderContext& grc, const Vec2i& topLeft, const Modality& modality) override
	{
		RenderControls(grc, topLeft);

		if (tabSelect < tabs.size())
		{
			RenderTab(grc, topLeft, tabs[tabSelect]);
		}
	}

	void RenderTabContent(IGuiRenderContext& grc, const GuiRect& contentRect, cstr panelKey)
	{
		UIPopulate populate;
		populate.renderElement = nullptr;
		populate.name = panelKey;
		publisher.Publish(populate);

		if (populate.renderElement)
		{
			populate.renderElement->Render(grc, contentRect);
		}
	}

	void RenderTab(IGuiRenderContext& grc, const Vec2i& topLeft, const Tab& tab)
	{
		GuiRect rect;
		GetRect(rect);

		GuiMetrics metrics;
		grc.Renderer().GetGuiMetrics(metrics);

		GuiRect absRect = GuiRect{ 0, 0, Width(rect), Height(rect) } + topLeft;

		Vec2i nw{ absRect.left, tab.lastRect.bottom + 2 };
		Vec2i lmid{ tab.lastRect.left, nw.y };
		Vec2i ne{ absRect.right-2, tab.lastRect.bottom + 2 };
		Vec2i rmid{ tab.lastRect.right - 2, nw.y };
		Vec2i sw{ nw.x, absRect.bottom - 3 };
		Vec2i se{ ne.x, absRect.bottom - 3 };

		GuiRect tabRect{ nw.x, nw.y, se.x, se.y };
		if (IsPointInRect(metrics.cursorPosition, tabRect) || IsPointInRect(metrics.cursorPosition, tab.lastRect))
		{
			Graphics::DrawLine(grc, 2, nw, lmid, Scheme().hi_topLeftEdge);
			Graphics::DrawLine(grc, 2, rmid, ne, Scheme().hi_topLeftEdge);
			Graphics::DrawLine(grc, 2, nw, sw, Scheme().hi_topLeftEdge);
			Graphics::DrawLine(grc, 2, sw, se + Vec2i{ 2, 0 }, Scheme().hi_bottomRightEdge);
			Graphics::DrawLine(grc, 2, se + Vec2i{0, 2}, ne - Vec2i{0, 0}, Scheme().hi_bottomRightEdge);

			Graphics::DrawLine(grc, 2, TopLeft(tab.lastRect), TopRight(tab.lastRect), Scheme().hi_topLeftEdge);
			Graphics::DrawLine(grc, 2, TopLeft(tab.lastRect), BottomLeft(tab.lastRect), Scheme().hi_topLeftEdge);
			Graphics::DrawLine(grc, 2, TopRight(tab.lastRect), BottomRight(tab.lastRect) + Vec2i{ 0,2 }, Scheme().hi_bottomRightEdge);
		}
		else
		{
			Graphics::DrawLine(grc, 2, nw, sw, Scheme().topLeftEdge);
			Graphics::DrawLine(grc, 2, sw, se + Vec2i{ 2, 0 }, Scheme().bottomRightEdge);
			Graphics::DrawLine(grc, 2, se + Vec2i{ 0, 2 }, ne - Vec2i{ 0, 0 }, Scheme().bottomRightEdge);

			Graphics::DrawLine(grc, 2, TopLeft(tab.lastRect), TopRight(tab.lastRect), Scheme().topLeftEdge);
			Graphics::DrawLine(grc, 2, TopLeft(tab.lastRect), BottomLeft(tab.lastRect), Scheme().topLeftEdge);
			Graphics::DrawLine(grc, 2, TopRight(tab.lastRect), BottomRight(tab.lastRect) + Vec2i{ 0,2 }, Scheme().bottomRightEdge);

			Graphics::DrawLine(grc, 2, nw, lmid, Scheme().topLeftEdge);
			Graphics::DrawLine(grc, 2, rmid, ne, Scheme().topLeftEdge);
		}

		RenderTabContent(grc, GuiRect{ nw.x + 4, nw.y + 4, se.x - 4, se.y - 4 }, tab.panelText.c_str());
	}
};

class Scrollbar: public IScrollbar
{
	int trapCount = 0;
	Vec2i grabPoint{ -1,-1 };
	bool isVertical = false;
	int32 grabPixelValue = 0;
	Vec2i span{ 0,0 };

	int32 minValue = 0;
	int32 maxValue = 0;
	int32 value = 0;
	int32 rowSize = 0;
	int32 pageSize = 0;

	IKeyboardSupervisor& keyboard;

	int32 PixelRange() const
	{
		return isVertical ? span.y : span.x;
	}

	int32 LogicalRange() const
	{
		return maxValue - minValue;
	}

	int32 PixelSelectValue(const Vec2i& relPos) const
	{
		return (isVertical) ? relPos.y : relPos.x;
	}

	int32 LogicalValue(int32 pixelValue)
	{
		float pr = (float)PixelRange();
		return pr == 0 ? 0 : (int32)((pixelValue / pr * (float)LogicalRange())) + minValue;
	}

	void CapValue(int32 candidateValue)
	{
		if (candidateValue < minValue) value = minValue;
		else if (candidateValue + pageSize > maxValue) value = maxValue - pageSize;
		else
		{
			value = candidateValue;
		}
	}

	int32 PixelValue(int32 logicalValue)
	{
		float lr = (float)LogicalRange();
		return lr == 0 ? 0 : (int32)((logicalValue / lr) * (float)PixelRange());
	}

	int32 PageSizeInPixels()
	{
		float lr = (float)LogicalRange();
		return lr == 0 ? 0 : (int32)((pageSize / lr) * (float)PixelRange());
	}

public:
	Scrollbar(IKeyboardSupervisor& _keyboard, bool _isVertical): 
		keyboard(_keyboard), isVertical(_isVertical)
	{
	}

	~Scrollbar()
	{
	}

	void Free() override
	{
		delete this;
	}

	void GetScroller(ScrollEvent& s)
	{
		s.logicalMinValue = minValue;
		s.logicalMaxValue = maxValue;
		s.logicalValue = value;
		s.logicalPageSize = pageSize;
		s.rowSize = rowSize;
	}

	void SetScroller(const ScrollEvent& s)
	{
		minValue = s.logicalMinValue;
		maxValue = s.logicalMaxValue;
		value = s.logicalValue;
		pageSize = s.logicalPageSize;
		rowSize = s.rowSize;
	}

	bool AppendEvent(const KeyboardEvent& k, ScrollEvent& updateStatus)
	{
		Key key = keyboard.GetKeyFromEvent(k);
		if (key.isPressed)
		{
			bool consume = true;

			if (Eq(key.KeyName, "HOME"))
			{
				value = minValue;
			}
			else if (Eq(key.KeyName, "END"))
			{
				value = maxValue - pageSize;
			}
			else if (Eq(key.KeyName, "PGUP"))
			{
				value -= pageSize;
			}
			else if (Eq(key.KeyName, "PGDOWN"))
			{
				value += pageSize;
			}
			else if (Eq(key.KeyName, "UP"))
			{
				value -= rowSize;
			}
			else if (Eq(key.KeyName, "DOWN"))
			{
				value += rowSize;
			}
			else
			{
				consume = false;
			}

			CapValue(value);

			if (consume)
			{
				updateStatus.logicalMinValue = minValue;
				updateStatus.logicalMaxValue = maxValue;
				updateStatus.logicalValue = value;
				updateStatus.logicalPageSize = pageSize;
				updateStatus.fromScrollbar = true;
				updateStatus.rowSize = rowSize;
			}

			return consume;
		}
		return false;
	}

	bool AppendEvent(const MouseEvent& me, const Vec2i& absTopLeft, ScrollEvent& updateStatus) override
	{
		int32 oldValue = value;

		if (me.HasFlag(me.LDown))
		{
			Vec2i ds = me.cursorPos - absTopLeft;
			int32 pixelValue = PixelSelectValue(ds);
			if (pixelValue < PixelValue(value))
			{
				value -= pageSize;
			}
			else if (pixelValue > PixelValue(value + pageSize))
			{
				value += pageSize;
			}
			else
			{
				grabPoint = me.cursorPos;
				grabPixelValue = PixelValue(value);
			}

			CapValue(value);
		}

		if (me.HasFlag(me.RDown))
		{
			Vec2i ds = me.cursorPos - absTopLeft;
			int32 pixelValue = PixelSelectValue(ds);
			if (pixelValue < PixelValue(value))
			{
				value -= rowSize;
			}
			else if (pixelValue > PixelValue(value + pageSize))
			{
				value += rowSize;
			}

			CapValue(value);
		}

		if (me.HasFlag(me.MouseWheel))
		{
			int32 delta = (int32)(((short)me.buttonData) / 120);
			value -= rowSize * delta;
			CapValue(value);
		}

		if (oldValue != value)
		{
			updateStatus.logicalMinValue = minValue;
			updateStatus.logicalMaxValue = maxValue;
			updateStatus.logicalValue = value;
			updateStatus.logicalPageSize = pageSize;
			updateStatus.fromScrollbar = true;
			updateStatus.rowSize = rowSize;
			return true;
		}

		if (me.HasFlag(me.LUp))
		{
			grabPoint = { -1,-1 };
			grabPixelValue = -1;
		}

		return false;
	}


	void Render(IGuiRenderContext& grc, const GuiRect& absRect, const Modality& modality, RGBAb hilightColour, RGBAb baseColour, RGBAb hilightEdge, RGBAb baseEdge, IEventCallback<ScrollEvent>& populator, EventId populationEventId)
	{
		GuiMetrics metrics;
		grc.Renderer().GetGuiMetrics(metrics);

		span = Span(absRect);

		if (LogicalRange() == 0)
		{
			ScrollEvent ev(populationEventId);
			ev.fromScrollbar = false;

			populator.OnEvent(ev);

			maxValue = ev.logicalMaxValue;
			minValue = ev.logicalMinValue;
			value = ev.logicalValue;
			pageSize = ev.logicalPageSize;
			rowSize = ev.rowSize;
		}

		bool lit = !modality.isUnderModal && IsPointInRect(metrics.cursorPosition, absRect);

		if (lit)
		{
			trapCount = 0;

			if (grabPoint.x >= 0)
			{
				Vec2i delta = metrics.cursorPosition - grabPoint;
				int32 dPixels = isVertical ? delta.y : delta.x;
				int32 pixelPos = grabPixelValue + dPixels;
				int32 newValue = LogicalValue(pixelPos);

				if (dPixels != 0)
				{
					if (newValue < minValue)
					{
						value = minValue;
						grabPoint = metrics.cursorPosition;
						grabPixelValue = 0;
					}
					else if (newValue + pageSize > maxValue)
					{
						value = maxValue - pageSize;
						grabPoint = metrics.cursorPosition;
						grabPixelValue = PixelValue(value);
					}
					else
					{
						value = newValue;
					}

					ScrollEvent s(populationEventId);
					s.logicalMinValue = minValue;
					s.logicalMaxValue = maxValue;
					s.logicalValue = value;
					s.logicalPageSize = pageSize;
					s.fromScrollbar = true;
					s.rowSize = rowSize;

					populator.OnEvent(s);
				}
			}
		}
		else
		{
			trapCount++;
			if (trapCount > 50)
			{
				trapCount = 0;
				grabPoint = { -1,-1 };
			}
		}

		Graphics::DrawRectangle(grc, absRect, lit ? hilightColour : baseColour, lit ? hilightColour : baseColour);
		Graphics::DrawBorderAround(grc, absRect, { 1,1 }, lit ? hilightEdge : baseEdge, lit ? hilightEdge : baseEdge);

		GuiRect sliderRect;

		if (isVertical)
		{
			sliderRect = GuiRect{ 1, 1, span.x - 2, PageSizeInPixels() } +Vec2i{ 0, PixelValue(value) } + TopLeft(absRect);
		}
		else
		{
			sliderRect = GuiRect{ 1, 1, PageSizeInPixels(), span.y - 2 } +Vec2i{ PixelValue(value), 0 } + TopLeft(absRect);
		}

		Graphics::DrawRectangle(grc, sliderRect, lit ? hilightColour : baseColour, lit ? hilightColour : baseColour);
		Graphics::DrawBorderAround(grc, sliderRect, { 1,1 }, lit ? hilightEdge : baseEdge, lit ? hilightEdge : baseEdge);
	}
};

class PanelScrollbar : public BasePanel, public IScroller, IObserver, IEventCallback<ScrollEvent>
{
	Scrollbar scrollbar;
	IPublisher& publisher;
	EventId setScrollId;
	EventId getScrollId;
	EventId uiScrollId;
	EventId routeKeyId;
	EventId routeMouseId;

	void OnEvent(Event& ev) override
	{
		if (ev.id == setScrollId)
		{
			auto& s = As<ScrollEvent>(ev);
			scrollbar.SetScroller(s);
		}
		else if (ev.id == getScrollId)
		{
			auto& s = As<ScrollEvent>(ev);
			scrollbar.GetScroller(s);
		}
		else if (ev.id == routeKeyId)
		{
			auto& r = As<RouteKeyboard>(ev);

			Events::ScrollEvent se(uiScrollId);
			if (scrollbar.AppendEvent(*r.ke, se))
			{
				r.consume = true;
				publisher.Publish(se);
			}
		}
		else if (ev.id == routeMouseId)
		{
			auto& r = As<RouteMouse>(ev);

			Events::ScrollEvent se(uiScrollId);
			if (scrollbar.AppendEvent(*r.me, r.absTopleft, se))
			{
				publisher.Publish(se);
			}
		}
	}

	void OnEvent(ScrollEvent& uiUpdate)
	{
		publisher.Publish(uiUpdate);
	}
public:
	PanelScrollbar(IPublisher& _publisher, IKeyboardSupervisor& keyboard, cstr _key, boolean32 isVertical) :
		scrollbar(keyboard, isVertical), publisher(_publisher),
		setScrollId(""_event), getScrollId(""_event), uiScrollId(""_event), routeKeyId(""_event), routeMouseId(""_event)
	{
		char eventText[256];

		{
			SecureFormat(eventText, sizeof(eventText), "%s_set", _key);
			EventId id = CreateEventIdFromVolatileString(eventText);
			memcpy(&setScrollId, &id, sizeof(id));
		}

		{
			SecureFormat(eventText, sizeof(eventText), "%s_get", _key);
			EventId id = CreateEventIdFromVolatileString(eventText);
			memcpy(&getScrollId, &id, sizeof(id));
		}

		{
			SecureFormat(eventText, sizeof(eventText), "%s_ui", _key);
			EventId id = CreateEventIdFromVolatileString(eventText);
			memcpy(&uiScrollId, &id, sizeof(id));
		}

		{
			SecureFormat(eventText, sizeof(eventText), "%s_sendkey", _key);
			EventId id = CreateEventIdFromVolatileString(eventText);
			memcpy(&routeKeyId, &id, sizeof(id));
		}

		{
			SecureFormat(eventText, sizeof(eventText), "%s_sendmouse", _key);
			EventId id = CreateEventIdFromVolatileString(eventText);
			memcpy(&routeMouseId, &id, sizeof(id));
		}

		publisher.Attach(this, getScrollId);
		publisher.Attach(this, setScrollId);
		publisher.Attach(this, routeKeyId);
		publisher.Attach(this, routeMouseId);
	}

	~PanelScrollbar()
	{
		publisher.Detach(this);
	}

	void Free() override
	{
		delete this;
	}

	IPane* Base() override
	{
		return this;
	}

	bool AppendEvent(const KeyboardEvent& k, const Vec2i& focusPoint, const Vec2i& absTopLeft) override
	{
		ScrollEvent updateStatus(uiScrollId);
		updateStatus.fromScrollbar = true;
		if (scrollbar.AppendEvent(k, updateStatus))
		{
			publisher.Publish(updateStatus);
			return true;
		}

		return false;
	}

	void AppendEvent(const MouseEvent& me, const Vec2i& absTopLeft) override
	{
		ScrollEvent updateStatus(uiScrollId);
		updateStatus.fromScrollbar = true;
		if (scrollbar.AppendEvent(me, absTopLeft, updateStatus))
		{
			publisher.Publish(updateStatus);
		}
	}

	void Render(IGuiRenderContext& grc, const Vec2i& topLeft, const Modality& modality) override
	{
		auto span = Span(ClientRect());
		GuiRect absRect = GuiRect{ 0, 0, span.x, span.y } +topLeft;
		scrollbar.Render(grc, absRect, modality, Scheme().hi_topLeft, Scheme().topLeft, Scheme().hi_topLeftEdge, Scheme().topLeftEdge, *this, uiScrollId);
	}
};

class PanelTextOutput : public BasePanel, public ITextOutputPane
{
	int32 fontIndex = 1;
	int32 horzAlign = 0;
	int32 vertAlign = 0;
	Vec2i padding{ 0,0 };
	IPublisher& publisher;
	EventId id;
public:
	PanelTextOutput(IPublisher& _publisher, int _fontIndex, cstr _key) :
		id(CreateEventIdFromVolatileString(_key)),
		publisher(_publisher), fontIndex(_fontIndex)
	{
	}

	~PanelTextOutput()
	{
	}

	void Free() override
	{
		delete this;
	}

	IPane* Base() override
	{
		return this;
	}

	void SetAlignment(int32 horz, int32 vert, int32 paddingX, int paddingY)
	{
		horzAlign = horz;
		vertAlign = vert;
		padding = { paddingX, paddingY };
	}

	bool AppendEvent(const KeyboardEvent& me, const Vec2i& focusPoint, const Vec2i& absTopLeft) override
	{
		return false;
	}

	void AppendEvent(const MouseEvent& me, const Vec2i& absTopLeft) override
	{

	}

	void Render(IGuiRenderContext& grc, const Vec2i& topLeft, const Modality& modality) override
	{
		RenderBackground(grc, topLeft, modality);

		auto span = Span(ClientRect());
		GuiRect absRect{ topLeft.x, topLeft.y, topLeft.x + span.x, topLeft.y + span.y };

		TextOutputEvent event(id);
		event.isGetting = true;
		*event.text = 0;
		publisher.Publish(event);

		RenderLabel(grc, event.text, absRect, horzAlign, vertAlign, padding, fontIndex, Scheme(), !modality.isUnderModal);
	}
};

Rococo::ILabelPane* PanelContainer::AddLabel(int32 fontIndex, const fstring& text, const GuiRect& rect)
{
   auto* label = new PanelLabel(platform.publisher, fontIndex, text);
   AddChild(label);
   label->SetRect(rect);
   return label;
}

Rococo::ISlider* PanelContainer::AddSlider(int32 fontIndex, const fstring& text, const GuiRect& rect, float minValue, float maxValue)
{
   auto* s = new PanelSlider(platform.publisher, platform.renderer, fontIndex, text, minValue, maxValue);
   AddChild(s);
   s->SetRect(rect);
   return s;
}


Rococo::ITabContainer* PanelContainer::AddTabContainer(int32 tabHeight, int32 fontIndex, const GuiRect& rect)
{
	auto* tabs = new TabContainer(platform.publisher, platform.keyboard, tabHeight, fontIndex);
	AddChild(tabs);
	tabs->SetRect(rect);
	return tabs;
}

Rococo::ITextOutputPane* PanelContainer::AddTextOutput(int32 fontIndex, const fstring& eventKey, const GuiRect& rect)
{
   auto* to = new PanelTextOutput(platform.publisher, fontIndex, eventKey);
   AddChild(to);
   to->SetRect(rect);
   return to;
}

Rococo::IRadioButton* PanelContainer::AddRadioButton(int32 fontIndex, const fstring& text, const fstring& key, const fstring& value, const GuiRect& rect)
{
   auto* radio = new PanelRadioButton(platform.publisher, fontIndex, text, key, value);
   AddChild(radio);
   radio->SetRect(rect);
   return radio;
}

Rococo::IScroller* PanelContainer::AddScroller(const fstring& key, const GuiRect& rect, boolean32 isVertical)
{
   auto* scroller = new PanelScrollbar(platform.publisher, platform.keyboard, key, isVertical);
   AddChild(scroller);
   scroller->SetRect(rect);
   return scroller;
}

IScrollbar* Utilities::CreateScrollbar(IKeyboardSupervisor& keyboard, bool _isVertical)
{
	return new Scrollbar(keyboard, _isVertical);
}

class ScriptedPanel : IEventCallback<ScriptCompileArgs>, IObserver, public IPaneBuilderSupervisor, public PanelContainer
{
	GuiRect lastRect{ 0, 0, 0, 0 };
	std::string scriptFilename;
	Platform& platform;
public:
	ScriptedPanel(Platform& _platform, cstr _scriptFilename) : PanelContainer(_platform),
		platform(_platform),
		scriptFilename(_scriptFilename)
	{
		platform.publisher.Attach(this, evFileUpdated);
	}

	~ScriptedPanel()
	{
		platform.publisher.Detach(this);
	}

	void OnEvent(Event& ev) override
	{
		if (ev.id == evFileUpdated)
		{
			auto& fue = As<FileUpdatedEvent>(ev);
			if (Rococo::Eq(fue.pingPath, scriptFilename.c_str()))
			{
				RefreshScript();
			}
		}
	}

	IPaneContainer* Root() override
	{
		return this;
	}

	void Free() override
	{
		delete this;
	}

	virtual void OnEvent(ScriptCompileArgs& args)
	{
		AddNativeCalls_RococoITabContainer(args.ss, nullptr);
		AddNativeCalls_RococoIPaneContainer(args.ss, nullptr);
		AddNativeCalls_RococoILabelPane(args.ss, nullptr);
		AddNativeCalls_RococoIPaneBuilder(args.ss, this);
		AddNativeCalls_RococoITextOutputPane(args.ss, nullptr);
		AddNativeCalls_RococoIRadioButton(args.ss, nullptr);
		AddNativeCalls_RococoIPane(args.ss, nullptr);
		AddNativeCalls_RococoISlider(args.ss, nullptr);
		AddNativeCalls_RococoIScroller(args.ss, nullptr);
	}

	void RefreshScript()
	{
		FreeAllChildren();
		platform.utilities.RunEnvironmentScript(platform, *this, scriptFilename.c_str(), false);
	}

	void Render(IGuiRenderContext& grc, const Vec2i& topLeft, const Modality& modality) override
	{
		if (IsVisible() && IsNormalized())
		{
			if (lastRect != BasePanel::ClientRect())
			{
				lastRect = BasePanel::ClientRect();
				RefreshScript();
			}
		}

		PanelContainer::Render(grc, topLeft, modality);
	}

	virtual IPanelSupervisor* Supervisor()
	{
		return this;
	}
};

IPaneBuilderSupervisor* GuiStack::BindPanelToScript(cstr scriptName)
{
   return new ScriptedPanel(*platform, scriptName);
}

struct PanelDelegate: public IPanelSupervisor
{
	IPaneBuilderSupervisor* current;

	void Free() override
	{
		delete this;
	}

	bool AppendEvent(const KeyboardEvent& me, const Vec2i& focusPoint, const Vec2i& absTopLeft) override
	{
		return current->Supervisor()->AppendEvent(me, focusPoint, absTopLeft);
	}

	void AppendEvent(const MouseEvent& me, const Vec2i& absTopLeft) override
	{
		current->Supervisor()->AppendEvent(me, absTopLeft);
	}

	const GuiRect& ClientRect() const override
	{
		return current->Supervisor()->ClientRect();
	}

	void SetScheme(const ColourScheme& scheme) override
	{
		current->Supervisor()->SetScheme(scheme);
	}

	const ColourScheme& Scheme() const override
	{
		return current->Supervisor()->Scheme();
	}

	IPanelSupervisor* operator[](int index) override
	{
		return (*current->Supervisor())[index];
	}

	int Children() const override
	{
		return current->Supervisor()->Children();
	}

	void AddChild(IPanelSupervisor* child) override
	{
		return current->Supervisor()->AddChild(child);
	}

	void RemoveChild(IPanelSupervisor* child) override
	{
		current->Supervisor()->RemoveChild(child);
	}

	void FreeAllChildren() override
	{
		current->Supervisor()->FreeAllChildren();
	}

	void SetColourBk1(RGBAb normal, RGBAb hilight) override
	{
		return current->Supervisor()->SetColourBk1(normal, hilight);
	}

	void SetColourBk2(RGBAb normal, RGBAb hilight) override
	{
		return current->Supervisor()->SetColourBk2(normal, hilight);
	}

	void SetColourEdge1(RGBAb normal, RGBAb hilight) override
	{
		return  current->Supervisor()->SetColourEdge1(normal, hilight);
	}

	void SetColourEdge2(RGBAb normal, RGBAb hilight) override
	{
		return  current->Supervisor()->SetColourEdge2(normal, hilight);
	}

	void SetColourFont(RGBAb normal, RGBAb hilight) override
	{
		return  current->Supervisor()->SetColourFont(normal, hilight);
	}

	boolean32/* isVisible */ IsVisible() override
	{
		return  current->Supervisor()->IsVisible();
	}

	boolean32/* isNormalized */ IsNormalized() override
	{
		return  current->Supervisor()->IsNormalized();
	}

	void SetVisible(boolean32 isVisible) override
	{
		return  current->Supervisor()->SetVisible(isVisible);
	}

	void GetRect(GuiRect& rect) override
	{
		return  current->Supervisor()->GetRect(rect);
	}

	void SetRect(const GuiRect& rect) override
	{
		return  current->Supervisor()->SetRect(rect);
	}

	void AlignLeftEdges(int32 x, boolean32 preserveSpan) override
	{
		return  current->Supervisor()->AlignLeftEdges(x, preserveSpan);
	}

	void AlignRightEdges(int32 x, boolean32 preserveSpan) override
	{
		return  current->Supervisor()->AlignRightEdges(x, preserveSpan);
	}

	void LayoutVertically(int32 vertBorder, int32 vertSpacing) override
	{
		return  current->Supervisor()->LayoutVertically(vertBorder, vertSpacing);
	}

	void SetCommand(int32 stateIndex, boolean32 deferAction, const fstring& text) override
	{
		return  current->Supervisor()->SetCommand(stateIndex, deferAction, text);
	}

	void SetPopulator(int32 stateIndex, const fstring& populatorName) override
	{
		return  current->Supervisor()->SetPopulator(stateIndex, populatorName);
	}

	void Render(IGuiRenderContext& grc, const Vec2i& topLeft, const Modality& modality) override
	{
		return current->Render(grc, topLeft, modality);
	}

	IPaneContainer* Root()
	{
		return current->Root();
	}
};

IPaneBuilderSupervisor* GuiStack::CreateOverlay()
{
	struct OverlayPanel: public IPaneBuilderSupervisor, PanelDelegate, public IUIElement, public IObserver
	{
		struct ANON: public IUIElement
		{
			Platform& platform;
			ANON(Platform& _platform) : platform(_platform) {}

			virtual bool OnKeyboardEvent(const KeyboardEvent& key)
			{
				return false;
			}

			virtual void OnRawMouseEvent(const MouseEvent& ev)
			{

			}

			virtual void OnMouseMove(Vec2i cursorPos, Vec2i delta, int dWheel)
			{

			}

			virtual void OnMouseLClick(Vec2i cursorPos, bool clickedDown)
			{
				if (!clickedDown)
				{
					platform.mathsVisitor.CancelSelect();
				}
			}

			virtual void OnMouseRClick(Vec2i cursorPos, bool clickedDown)
			{
				if (!clickedDown)
				{
					platform.mathsVisitor.CancelSelect();
				}
			}

			virtual void Render(IGuiRenderContext& rc, const GuiRect& absRect)
			{
				
			}
		} textureCancel;

		AutoFree<IPaneBuilderSupervisor> tabbedPanel;
		AutoFree<IPaneBuilderSupervisor> txFocusPanel;
		Platform& platform;
		EventId stnId = "selected.texture.name"_event;

		OverlayPanel(Platform& _platform) :
			platform(_platform),
			textureCancel(_platform),
			tabbedPanel(new ScriptedPanel(_platform, "!scripts/panel.overlay.sxy")),
			txFocusPanel(new ScriptedPanel(_platform, "!scripts/panel.texture.sxy"))
		{
			current = tabbedPanel;
			platform.gui.RegisterPopulator("texture_view", this);
			platform.gui.RegisterPopulator("texture_cancel", &textureCancel);
			platform.publisher.Attach(this, "selected.texture.name"_event);
		}

		~OverlayPanel()
		{
			platform.gui.UnregisterPopulator(&textureCancel);
			platform.gui.UnregisterPopulator(this);
			platform.publisher.Detach(this);
		}

		void OnEvent(Event& ev) override
		{
			if (ev.id == stnId)
			{
				auto& toe = As<TextOutputEvent>(ev);
				if (toe.isGetting)
				{
					auto* key = platform.mathsVisitor.SelectedKey();
					if (key)
					{
						if (StartsWith(key, "MatId "))
						{
							int index = atoi(key + 6);
							auto id = (MaterialId) index;
							auto name = platform.renderer.GetMaterialTextureName(id);

							if (!name) name = "--unknown--";
							SafeFormat(toe.text, sizeof(toe.text), " %s: %s", key, name);
						}
						else
						{
							SafeFormat(toe.text, sizeof(toe.text), " %s", key);
						}
					}
				}
			}
		}

		void Free() override
		{
			delete this;
		}

		virtual bool OnKeyboardEvent(const KeyboardEvent& key)
		{
			return false;
		}

		virtual void OnRawMouseEvent(const MouseEvent& ev)
		{

		}

		virtual void OnMouseMove(Vec2i cursorPos, Vec2i delta, int dWheel)
		{

		}

		virtual void OnMouseLClick(Vec2i cursorPos, bool clickedDown)
		{

		}

		virtual void OnMouseRClick(Vec2i cursorPos, bool clickedDown)
		{

		}

		virtual void Render(IGuiRenderContext& rc, const GuiRect& absRect)
		{
			auto* key = platform.mathsVisitor.SelectedKey();
			if (key)
			{
				auto* ext = Rococo::GetFileExtension(key);
				if (StartsWith(key, "MatId "))
				{
					int index = atoi(key + 6);
					MaterialId id = (MaterialId)index;
					Graphics::RenderBitmap_ShrinkAndPreserveAspectRatio(rc, id, absRect);
				}
				else if (StartsWith(key, "TxId "))
				{
					int index = atoi(key + 5);

					SpriteVertexData ignore{ 0.0f, 0.0f, 0.0f, 0.0f };
					RGBAb none(0, 0, 0, 0);

					GuiVertex quad[6] =
					{
						{
							{ (float)absRect.left, (float)absRect.top },
							{ { 0, 0 }, 0 },
							ignore,
							none
						},
						{
							{ (float)absRect.right, (float)absRect.top },
							{ { 1, 0 }, 0 },
							ignore,
							none
						},
						{
							{ (float)absRect.right,(float)absRect.bottom },
							{ { 1, 1 }, 0 },
							ignore,
							none
						},
						{
							{ (float)absRect.right, (float)absRect.bottom },
							{ { 1, 1 }, 0 },
							ignore,
							none
						},
						{
							{ (float)absRect.left, (float)absRect.bottom },
							{ { 0, 1 }, 0 },
							ignore,
							none
						},
						{
							{ (float)absRect.left, (float)absRect.top },
							{ { 0, 0 }, 0 },
							ignore,
							none
						}
					};

					rc.DrawCustomTexturedMesh(absRect, ID_TEXTURE(index), "!r32f.ps", quad, 6);
					//	if (id) Graphics::RenderBitmap_ShrinkAndPreserveAspectRatio(rc, id, absRect);
				}
			}
		}

		void Render(IGuiRenderContext& grc, const Vec2i& topLeft, const Modality& modality) override
		{
			auto* key = platform.mathsVisitor.SelectedKey();
			auto* value = platform.mathsVisitor.SelectedValue();

			if (key != nullptr)
			{
				current = txFocusPanel;
			}
			else
			{
				current = tabbedPanel;
			}

			GuiMetrics metrics;
			grc.Renderer().GetGuiMetrics(metrics);

			GuiRect fullScreen = { 0, 0, metrics.screenSpan.x, metrics.screenSpan.y };
			SetRect(fullScreen);

			return current->Render(grc, topLeft, modality);
		}

		IPanelSupervisor* Supervisor() override
		{
			return this;
		}

		virtual Rococo::IPaneContainer* Root()
		{
			return PanelDelegate::Root();
		}
	};
	return new OverlayPanel(*platform);
}

namespace Rococo
{
	namespace Graphics
	{
		IMeshBuilderSupervisor* CreateMeshBuilder(IRenderer& renderer);
	}
}

struct OSVenue : public IMathsVenue
{
	int numProcessors;
	HANDLE hCurrentProcess;

	FILETIME createdAt;
	FILETIME exitAt;
	FILETIME kernelTime;
	FILETIME userTime;

	ULARGE_INTEGER lastCPU;
	ULARGE_INTEGER  lastSysCPU;
	ULARGE_INTEGER  lastUserCPU;

	OSVenue()
	{
		SYSTEM_INFO sysInfo;
		FILETIME ftime;

		GetSystemInfo(&sysInfo);
		numProcessors = sysInfo.dwNumberOfProcessors;

		GetSystemTimeAsFileTime(&ftime);
		memcpy(&lastCPU, &ftime, sizeof(FILETIME));

		hCurrentProcess = GetCurrentProcess();
		GetProcessTimes(hCurrentProcess, &createdAt, &exitAt, &kernelTime, &userTime);
		memcpy(&lastSysCPU, &kernelTime, sizeof(FILETIME));
		memcpy(&lastUserCPU, &userTime, sizeof(FILETIME));

		lastCPU = lastSysCPU;
	}

	double usage = 0;

	double getCurrentValue() 
	{
		FILETIME ftime, fsys, fuser;
		ULARGE_INTEGER now, sys, user;
		double percent;

		GetSystemTimeAsFileTime(&ftime);
		memcpy(&now, &ftime, sizeof(FILETIME));

		if (!GetProcessTimes(hCurrentProcess, &ftime, &ftime, &fsys, &fuser))
		{
			return usage;
		}

		memcpy(&sys, &fsys, sizeof(FILETIME));
		memcpy(&user, &fuser, sizeof(FILETIME));
		percent = (double) ((sys.QuadPart - lastSysCPU.QuadPart) + (user.QuadPart - lastUserCPU.QuadPart));
		percent /= (now.QuadPart - lastCPU.QuadPart);
		percent /= numProcessors;

		if ((now.QuadPart - lastCPU.QuadPart) < 20000000)
		{
			return usage;
		}

		lastCPU = now;
		lastUserCPU = user;
		lastSysCPU = sys;

		usage = percent * 100;
		return usage;
	}

	virtual void ShowVenue(IMathsVisitor& visitor)
	{
		visitor.ShowString("OS", "Windows");

		SYSTEMTIME sysTime;
		GetSystemTime(&sysTime);

		visitor.ShowString("Time", "%.2d/%.2d/%.4d %.2d:%.2d:%.2d", sysTime.wDay, sysTime.wMonth, sysTime.wYear, sysTime.wHour, sysTime.wMinute, sysTime.wSecond);
		visitor.ShowDecimal("CPU Time", OS::CpuTicks());
		visitor.ShowDecimal("CPU Hz", OS::CpuHz());
		visitor.ShowDecimal("UTC Ticks", OS::UTCTime());

		MEMORYSTATUSEX statex = { 0 };
		statex.dwLength = sizeof(statex);
		GlobalMemoryStatusEx(&statex);

		visitor.ShowString("Memory used", "%lld MB of %lld MB. Load %d%%", (statex.ullTotalPhys - statex.ullAvailPhys) / 1048576, statex.ullTotalPhys / 1048576, statex.dwMemoryLoad);

		int32 processorNumber = (int32)GetCurrentProcessorNumber();
		visitor.ShowString("Cpu # (Main Thread)","%d", processorNumber);
		visitor.ShowString("Process Id", "%d", (int32)GetCurrentProcessId());

		HANDLE hProcess = GetCurrentProcess();

		DWORD handleCount;
		GetProcessHandleCount(hProcess, &handleCount);
		
		visitor.ShowString("Handle Count", "%d", (int32)handleCount);

		visitor.ShowString("Command Line", "%s", (cstr) GetCommandLineA());

		PROCESS_MEMORY_COUNTERS pmc;
		if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc)))
		{
			visitor.ShowString("Page Faults", "%u", pmc.PageFaultCount);
			visitor.ShowString("PageFile usage", "%llu kb", pmc.PagefileUsage / 1024);
			visitor.ShowString("Working Set Size", "%llu kb", pmc.WorkingSetSize / 1024);
			visitor.ShowString("Peak PageFile usage", "%llu kb", pmc.PeakPagefileUsage / 1024);
			visitor.ShowString("Peak Working Set Size", "%llu kb", pmc.PeakWorkingSetSize / 1024);	
		}

		double pc = getCurrentValue();

		visitor.ShowString("User CPU Load", "%3.1lf%%", pc);
	}
};

struct PlatformTabs: IObserver, IUIElement, public IMathsVenue
{
	Platform& platform;
	IMathsVenue* venue = nullptr;
	OSVenue osVenue;

	PlatformTabs(Platform& _platform): platform(_platform)
	{
		platform.publisher.Attach(this, UIPopulate::EvId());
	}

	~PlatformTabs()
	{
		platform.publisher.Detach(this);
	}


	virtual void ShowVenue(IMathsVisitor& visitor)
	{
		platform.renderer.ShowWindowVenue(visitor);
	}

	void OnEvent(Event& ev) override
	{
		auto& pop = As<UIPopulate>(ev);

		venue = nullptr;

		if (Eq(pop.name, "overlay.window"))
		{
			pop.renderElement = this;
			venue = this;
		}
		else if (Eq(pop.name, "overlay.renderer"))
		{
			pop.renderElement = this;
			venue = platform.renderer.Venue();
		}
		else if (Eq(pop.name, "overlay.camera"))
		{
			pop.renderElement = this;
			venue = &platform.camera.Venue();
		}
		else if (Eq(pop.name, "overlay.textures"))
		{
			pop.renderElement = this;
			venue = platform.renderer.TextureVenue();
		}
		else if (Eq(pop.name, "overlay.meshes"))
		{
			pop.renderElement = this;
			venue = platform.meshes.Venue();
		}
		else if (Eq(pop.name, "overlay.os"))
		{
			pop.renderElement = this;
			venue = &osVenue;
		}
		else if (Eq(pop.name, "overlay.cache"))
		{
			pop.renderElement = this;
			venue = platform.sourceCache.Venue();
		}
		else
		{
		}
	}

	bool OnKeyboardEvent(const KeyboardEvent& key)  override
	{
		return platform.mathsVisitor.AppendKeyboardEvent(key);
	}

	void OnRawMouseEvent(const MouseEvent& me) override
	{
		platform.mathsVisitor.AppendMouseEvent(me);
	}

	void OnMouseMove(Vec2i cursorPos, Vec2i delta, int dWheel)  override
	{

	}

	void OnMouseLClick(Vec2i cursorPos, bool clickedDown) override
	{

	}

	void OnMouseRClick(Vec2i cursorPos, bool clickedDown)  override
	{

	}

	void Render(IGuiRenderContext& rc, const GuiRect& absRect)  override
	{
		platform.mathsVisitor.Clear();
		if (venue) venue->ShowVenue(platform.mathsVisitor);
		platform.mathsVisitor.Render(rc, absRect, 4);
	}

};

void Main(HANDLE hInstanceLock, IAppFactory& appFactory, cstr title)
{
	AutoFree<IOSSupervisor> os = GetOS();
	AutoFree<IInstallationSupervisor> installation = CreateInstallation("content.indicator.txt", *os);
	AutoFree<Rococo::Events::IPublisherSupervisor> publisher(Events::CreatePublisher());
	os->Monitor(installation->Content());

	OS::PrintDebug("Starting mainWindow!\n");

	AutoFree<IDX11Window> mainWindow(CreateDX11Window(*installation));

	SetWindowTextA(mainWindow->Window(), title);

	AutoFree<ISourceCache> sourceCache(CreateSourceCache(*installation));
	AutoFree<IDebuggerWindow> debuggerWindow(Windows::IDE::CreateDebuggerWindow(mainWindow->Window()));

	Rococo::M::InitScriptSystem(*installation);

	AutoFree<Graphics::IMeshBuilderSupervisor> meshes = Graphics::CreateMeshBuilder(mainWindow->Renderer());
	AutoFree<Entities::IInstancesSupervisor> instances = Entities::CreateInstanceBuilder(*meshes, mainWindow->Renderer(), *publisher);
	AutoFree<Entities::IMobilesSupervisor> mobiles = Entities::CreateMobilesSupervisor(*instances);
	AutoFree<Graphics::ICameraSupervisor> camera = Graphics::CreateCamera(*instances, *mobiles, mainWindow->Renderer());
	AutoFree<Graphics::ISceneSupervisor> scene = Graphics::CreateScene(*instances, *camera);
	AutoFree<IKeyboardSupervisor> keyboard = CreateKeyboardSupervisor();
	AutoFree<Graphics::ISpriteSupervisor> sprites = Graphics::CreateSpriteSupervisor(mainWindow->Renderer());
	AutoFree<IConfigSupervisor> config = CreateConfig();
	AutoFree<Graphics::IRimTesselatorSupervisor> rimTesselator = Graphics::CreateRimTesselator();;

	Utilities utils(*installation, mainWindow->Renderer());

	AutoFree<IMathsVisitorSupervisor> mathsVisitor = CreateMathsVisitor(utils, *keyboard);

	GuiStack gui(*publisher, *sourceCache, mainWindow->Renderer(), utils);

	Tesselators tesselators{ *rimTesselator };
	Platform platform{ *os, *installation, mainWindow->Renderer(), *sourceCache, *debuggerWindow, *publisher, utils, gui, *keyboard, *config, *meshes, *instances, *mobiles, *sprites, *camera, *scene, tesselators, *mathsVisitor, title };
	gui.platform = &platform;

	AutoFree<IApp> app(appFactory.CreateApp(platform));

	PlatformTabs tabs(platform);

	app->OnCreate();
	mainWindow->Run(hInstanceLock, *app);
}

namespace Rococo
{
	Events::EventId UIInvoke::EvId()
	{
		static EventId invokeEvent = "ui.invoke"_event;
		return invokeEvent;
	}

	UIInvoke::UIInvoke() : Event(EvId())
	{

	}

	UIPopulate::UIPopulate() : Event(EvId())
	{

	}

	Events::EventId UIPopulate::EvId()
	{
		static EventId invokeEvent = "ui.populate"_event;
		return invokeEvent;
	}

	int M_Platorm_Win64_Main(HINSTANCE hInstance, IAppFactory& factory, cstr title, HICON hLarge, HICON hSmall)
	{
		rchar filename[1024];
		GetModuleFileNameA(nullptr, filename, 1024);
		for (char* p = filename; *p != 0; p++)
		{
			if (*p == '\\') *p = '#';
		}

		HANDLE hInstanceLock = CreateEventA(nullptr, TRUE, FALSE, filename);

		int err = GetLastError();
		if (err == ERROR_ALREADY_EXISTS)
		{
			SetEvent(hInstanceLock);

			if (IsDebuggerPresent())
			{
				ShowMessageBox(Windows::NoParent(), "Application is already running", filename, MB_ICONEXCLAMATION);
			}

			return err;
		}

		int errCode = 0;

		try
		{
			InitRococoWindows(hInstance, hLarge, hSmall, nullptr, nullptr);
			Main(hInstanceLock, factory, title);
		}
		catch (IException& ex)
		{
			rchar text[256];
			SafeFormat(text, 256, "%s crashed", title);
			OS::ShowErrorBox(NoParent(), ex, text);
			errCode = ex.ErrorCode();
		}

		CloseHandle(hInstanceLock);

		return errCode;
	}
}