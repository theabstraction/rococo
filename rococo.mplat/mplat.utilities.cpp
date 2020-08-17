#include "rococo.mplat.h"

#include <rococo.os.win32.h>
#include <rococo.window.h>

#include <Commdlg.h>

#include <vector>
#include <string>
#include <unordered_map>
#include <rococo.variable.editor.h>

#include <rococo.file.browser.h>

#include <algorithm>

#define ROCOCO_USE_SAFE_V_FORMAT
#include <rococo.strings.h>

using namespace Rococo;
using namespace Rococo::Events;

static auto evFileUpdated = "OnFileUpdated"_event;

namespace Rococo
{
	namespace MPlatImpl
	{
		IScrollbar* CreateScrollbar(bool _isVertical);
	}
}

namespace Rococo
{
	namespace MPlatImpl
	{
		void RunEnvironmentScript(ScriptPerformanceStats& stats, Platform& platform, IEventCallback<ScriptCompileArgs>& _onScriptEvent, const char* name, bool addPlatform, bool shutdownOnFail, bool trace);
	}
}

class Utilities :
	public IUtilitiesSupervisor,
	public IMathsVenue,
	public IObserver,
	public Browser::IBrowserFileChangeNotification,
	public IContextMenuEvents
{
	IInstallation& installation;
	IRenderer& renderer;
	AutoFree<Graphics::ITextTesselatorSupervisor> textTesselator;
	Platform* platform = nullptr;
	AutoFree<Graphics::IHQFontsSupervisor> hqFonts;
public:
	Utilities(IInstallation& _installation, IRenderer& _renderer) : installation(_installation), renderer(_renderer)
	{
	}

	~Utilities()
	{
		platform->publisher.Unsubscribe(this);
	}

	Rococo::Graphics::IHQFonts& GetHQFonts()
	{
		return *hqFonts;
	}

	void Free() override
	{
		delete this;
	}

	void SetPlatform(Platform& platform) override
	{
		this->platform = &platform;
		textTesselator = Graphics::CreateTextTesselator(platform);
		hqFonts = Graphics::CreateHQFonts(platform.renderer);
		platform.publisher.Subscribe(this, evUIInvoke);
	}

	void ShowBusy(bool enable, cstr title, cstr messageFormat, ...)
	{
		Rococo::Events::BusyEvent busy;
		busy.isNowBusy = enable;
		busy.message = title;

		va_list args;
		va_start(args, messageFormat);
		SafeVFormat(busy.resourceName, sizeof(busy.resourceName), messageFormat, args);
		platform->publisher.Publish(busy, Rococo::Events::evBusy);
	}

	void OnEvent(Event& ev) override
	{
		auto& args = As<UIInvoke>(ev);
		if (Eq(args.command, "file.browswer.ui.select"))
		{
			OnBrowserSelect();
		}
		else if (Eq(args.command, "file.browswer.ui.cancel"))
		{
			OnBrowserCancel();
		}
	}

	IScrollbar* CreateScrollbar(bool _isVertical) override;

	Graphics::ITextTesselator& GetTextTesselator() override
	{
		return *textTesselator;
	}

	fstring ToShortString(Graphics::MaterialCategory value) const override
	{
		return Graphics::ToShortString(value);
	}

	IMathsVenue* Venue() override
	{
		return this;
	}

	IBloodyPropertySetEditorSupervisor* CreateBloodyPropertySetEditor(IEventCallback<BloodyNotifyArgs>& _onDirty) override
	{
		return Rococo::CreateBloodyPropertySetEditor(*platform, _onDirty);
	}

	void AddSubtitle(cstr subtitle)
	{
		char fullTitle[256];

		if (subtitle && subtitle[0])
		{
			SafeFormat(fullTitle, sizeof(fullTitle), "%s - %s", platform->title, subtitle);
		}
		else
		{
			SafeFormat(fullTitle, sizeof(fullTitle), "%s", platform->title);
		}

		SetWindowTextA(platform->renderer.Window(), fullTitle);
	}

	bool GetLoadLocation(Windows::IWindow& parent, LoadDesc& ld) override
	{
		wchar_t filter[128];
		SecureFormat(filter, 128, L"%S%c%S%c%c", ld.extDesc, 0, ld.ext, 0, 0);

		OPENFILENAMEW dialog = { 0 };
		dialog.lStructSize = sizeof(dialog);
		dialog.hwndOwner = parent;
		dialog.lpstrFilter = filter;
		dialog.nFilterIndex = 1;
		dialog.lpstrFile = ld.path;
		dialog.nMaxFile = Rococo::IO::MAX_PATHLEN;

		wchar_t u16Caption[256];
		SafeFormat(u16Caption, 256, L"%S", ld.caption);

		dialog.lpstrTitle = u16Caption;
		dialog.Flags = OFN_CREATEPROMPT | OFN_ENABLESIZING | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;

		wchar_t u16Ext[64];
		SafeFormat(u16Caption, 64, L"%S", ld.ext);

		dialog.lpstrDefExt = u16Ext;

		renderer.SwitchToWindowMode();

		if (GetOpenFileNameW(&dialog))
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
		wchar_t filter[128];
		SecureFormat(filter, 128, L"%S%c%S%c%c", sd.extDesc, 0, sd.ext, 0, 0);

		OPENFILENAMEW dialog = { 0 };
		dialog.lStructSize = sizeof(dialog);
		dialog.hwndOwner = parent;
		dialog.lpstrFilter = filter;
		dialog.nFilterIndex = 1;
		dialog.lpstrFile = sd.path;
		dialog.nMaxFile = sizeof(sd.path) / sizeof(wchar_t);

		wchar_t u16Caption[256];
		SafeFormat(u16Caption, 256, L"%S", sd.caption);
		dialog.lpstrTitle = u16Caption;

		wchar_t u16Ext[256];
		SafeFormat(u16Ext, 256, L"%S", sd.ext);
		dialog.lpstrDefExt = u16Ext;

		wchar_t initialPath[IO::MAX_PATHLEN];

		size_t len = wcslen(sd.path);

		if (sd.path[len - 1] == '\\')
		{
			SafeFormat(initialPath, IO::MAX_PATHLEN, L"%s", sd.path);
			*sd.path = 0;
		}

		renderer.SwitchToWindowMode();

		if (GetSaveFileNameW(&dialog))
		{
			sd.shortName = sd.path + dialog.nFileOffset;
			return true;
		}
		else
		{
			int error = CommDlgExtendedError();
			if (error != 0) Throw(error, "Error GetSaveFileNameA");
			return false;
		}
	}

	void EnumerateFiles(IEventCallback<const wchar_t*>& cb, cstr pingPathDirectory) override
	{
		struct : IEventCallback<IO::FileItemData>
		{
			std::vector<std::wstring> allResults;
			void OnEvent(IO::FileItemData& file) override
			{
				allResults.push_back(file.fullPath);
			}
		} addToList;

		if (pingPathDirectory == nullptr || pingPathDirectory[0] != '!')
		{
			Throw(0, "Directories must be inside the content directory. Use the '!<directory>' notation");
		}

		wchar_t shortDir[IO::MAX_PATHLEN];
		wchar_t directory[IO::MAX_PATHLEN];

		SafeFormat(shortDir, IO::MAX_PATHLEN, L"%S", pingPathDirectory);

		EndDirectoryWithSlash(shortDir, IO::MAX_PATHLEN);

		SafeFormat(directory, IO::MAX_PATHLEN, L"%s%s", (cstr)installation.Content(), shortDir + 1);
		IO::ForEachFileInDirectory(directory, addToList, true);

		std::sort(addToList.allResults.begin(), addToList.allResults.end());

		for (auto& s : addToList.allResults)
		{
			wchar_t contentRelativePath[IO::MAX_PATHLEN];
			SafeFormat(contentRelativePath, IO::MAX_PATHLEN, L"%s%s", shortDir, s.c_str());
			cb.OnEvent(contentRelativePath);
		}
	}

	bool QueryYesNo(Windows::IWindow& parent, cstr question, cstr caption) override
	{
		cstr title = caption == nullptr ? platform->title : caption;
		renderer.SwitchToWindowMode();
		return ShowMessageBox(parent, question, title, MB_ICONQUESTION | MB_YESNO) == IDYES;
	}

	struct PerformanceStats
	{
		OS::ticks totalLoadCost = 0;
		OS::ticks totalCompileCost = 0;
		OS::ticks totalExecuteCost = 0;
		int64 moduleCallCount = 0;
	};

	std::unordered_map<std::string, PerformanceStats> nameToStats;

	struct NameAndStats
	{
		U8FilePath name;
		PerformanceStats stats;
	};
	std::vector<NameAndStats> nameAndStats;

	void ShowVenue(IMathsVisitor& visitor) override
	{
		visitor.ShowString("", "Loading  Compiling  Execution Call-count");

		nameAndStats.reserve(nameToStats.size());
		nameAndStats.clear();

		for (auto i : nameToStats)
		{
			NameAndStats nas;
			SafeFormat(nas.name.buf, nas.name.CAPACITY, "%s", i.first.c_str());
			nas.stats = i.second;
			nameAndStats.push_back(nas);
		}

		std::sort(nameAndStats.begin(), nameAndStats.end(),
			[](const NameAndStats& a, const NameAndStats& b)
			{
				return strcmp(a.name, b.name) < 0;
			}
		);

		for (auto i : nameAndStats)
		{
			float cyclesPerMs = OS::CpuHz() / 1000.0f;
			float loadCost = i.stats.totalLoadCost / cyclesPerMs;
			float compileCost = i.stats.totalCompileCost / cyclesPerMs;
			float executeCost = i.stats.totalExecuteCost / cyclesPerMs;
			visitor.ShowString(i.name, " %4.0fms     %4.0fms     %4.0fms     %d", loadCost, compileCost, executeCost, i.stats.moduleCallCount);
		}
	}

	void RunEnvironmentScript(IEventCallback<ScriptCompileArgs>& _onScriptEvent, const char* name, bool addPlatform, bool shutdownOnFail, bool trace) override
	{
		ScriptPerformanceStats stats = { 0 };
		Rococo::MPlatImpl::RunEnvironmentScript(stats, *platform, _onScriptEvent, name, addPlatform, shutdownOnFail, trace);

		auto i = nameToStats.find(name);
		if (i == nameToStats.end())
		{
			i = nameToStats.insert(std::make_pair(name, PerformanceStats{})).first;
		}

		i->second.totalCompileCost += stats.compileTime;
		i->second.totalExecuteCost += stats.executeTime;
		i->second.totalLoadCost += stats.loadTime;
		i->second.moduleCallCount = i->second.moduleCallCount + 1;
	}

	void ShowErrorBox(Windows::IWindow& parent, IException& ex, cstr message) override
	{
		renderer.SwitchToWindowMode();
		OS::ShowErrorBox(parent, ex, message);
	}

	void RefreshResource(cstr pingPath) override
	{
		FileUpdatedEvent fileUpdated;
		fileUpdated.pingPath = pingPath;

		platform->sourceCache.Release(pingPath);

		platform->publisher.Publish(fileUpdated, evFileUpdated);
	}

	IVariableEditor* CreateVariableEditor(Windows::IWindow& window, const Vec2i& span, int32 labelWidth, cstr appQueryName, cstr defaultTab, cstr defaultTooltip, IVariableEditorEventHandler* eventHandler, const Vec2i* topLeft) override
	{
		renderer.SwitchToWindowMode();
		return Rococo::CreateVariableEditor(window, span, labelWidth, appQueryName, defaultTab, defaultTooltip, eventHandler, topLeft);
	}

	virtual void SaveBinary(const wchar_t* pathname, const void* buffer, size_t nChars)
	{
		FileHandle fh = CreateFileW(pathname, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (fh == INVALID_HANDLE_VALUE)
		{
			Throw(GetLastError(), "Error saving %S", pathname);
		}

		if (!WriteFile(fh, buffer, (DWORD)nChars, nullptr, nullptr))
		{
			Throw(GetLastError(), "Error writing %S", pathname);
		}
	}

	AutoFree<IMPlatFileBrowser> browser;
	AutoFree<IPaneBuilderSupervisor> browsingPane;

	void BrowseFiles(IBrowserRulesFactory& factory)  override
	{
		if (!browser)
		{
			browser = CreateMPlatFileBrowser(platform->publisher, platform->installation, platform->gui, platform->keyboard, *this);
		}

		if (!browsingPane)
		{
			browsingPane = platform->gui.BindPanelToScript(factory.GetPanePingPath());
		}

		browser->Engage(factory);

		platform->gui.PushTop(browsingPane->Supervisor(), true);
	}

	void OnFileSelect(const U32FilePath& path, bool doubleClick)
	{
		if (doubleClick)
		{
			OnBrowserSelect();
		}
	}

	void OnBrowserSelect()
	{
		auto* old = platform->gui.Pop();

		// Assume the top is a file browser
		if (browser->Select())
		{
			browser = nullptr;
			browsingPane = nullptr;
		}
		else
		{
			platform->gui.PushTop(old, true);
		}
	}

	void OnBrowserCancel()
	{
		// Assume the top is a file browser
		platform->gui.Pop();
		browser = nullptr;
		browsingPane = nullptr;
	}

	AutoFree<IPaneBuilderSupervisor> contextMenuPane;
	AutoFree<IContextMenuSupervisor> contextMenu;

	EventIdRef evGetPopupRef = "mplat.default.contextmenu"_event;

	IContextMenuSupervisor& GetContextMenu()
	{
		if (!contextMenu)
		{
			contextMenu = MPlatImpl::CreateContextMenu(platform->publisher, *this);
		}

		return *contextMenu;
	}

	void CloseContextMenu()
	{
		if (platform->gui.Top() == contextMenuPane->Supervisor())
		{
			platform->gui.Pop();
		}
		else
		{
			Throw(0, "Expecting context menu to be the top level of the gui stack");
		}
	}

	/* IContextMenuEvent::OnItemSelected */
	void OnItemSelected(IContextMenuSupervisor& cm) override
	{
		CloseContextMenu();
	}

	/* IContextMenuEvent::OnClickOutsideControls */
	void OnClickOutsideControls(IContextMenuSupervisor& menu) override
	{
		CloseContextMenu();
	}

	IContextMenu& PopupContextMenu()
	{
		IContextMenuSupervisor& cm = GetContextMenu();

		if (!contextMenuPane)
		{
			contextMenuPane = platform->gui.BindPanelToScript("!scripts/panel.context-menu.sxy");
		}

		platform->gui.PushTop(contextMenuPane->Supervisor(), true);

		return cm;
	}
};

namespace Rococo
{
	namespace MPlatImpl
	{
		IUtilitiesSupervisor* CreateUtilities(IInstallation& installation, IRenderer& renderer)
		{
			return new Utilities(installation, renderer);
		}
	}
}

IScrollbar* Utilities::CreateScrollbar(bool _isVertical)
{
	return Rococo::MPlatImpl::CreateScrollbar(_isVertical);
}