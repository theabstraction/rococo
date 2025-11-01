#include "rococo.mplat.h"
# define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers
# define NOMINMAX // Macros min(a, b) and max(a, b)
# define NOCOMM // COMM driver routines
# define NOKANJI // Kanji support stuff.
# define NOHELP // Help engine interface.
# define NOPROFILER // Profiler interface.
# define NODEFERWINDOWPOS // DeferWindowPos routines
# define NOMCX // Modem Configuration Extensions

# include <Windows.h>
# include <rococo.window.h>

#include <rococo.hashtable.h>
#include <rococo.variable.editor.h>
#include <rococo.file.browser.h>
#include <rococo.sexy.api.h>
#include <rococo.time.h>
#include <rococo.io.h>
#include <mplat/mplat.events.h>


#include <Commdlg.h>
#include <vector>
#include <algorithm>
#include <string>

using namespace Rococo;
using namespace Rococo::Events;
using namespace Rococo::Strings;
using namespace Rococo::Graphics;

static auto evFileUpdated = "OnFileUpdated"_event;

namespace Rococo::MPlatImpl
{
	GUI::IScrollbar* CreateScrollbar(bool _isVertical);
	// Note - implicityIncludes is NULL, MPlat defaults are used, which may conflict with security.
	void RunEnvironmentScriptImpl(ScriptPerformanceStats& stats, Platform& platform, IScriptEnumerator* implicitIncludes, IScriptCompilationEventHandler& _onScriptEvent, const char* name, bool addPlatform, bool shutdownOnFail, bool trace, int32 id, Strings::IStringPopulator* onScriptCrash, StringBuilder* declarationBuilder);
}

class FileHandle
{
	HANDLE hFile;
public:
	FileHandle(HANDLE _hFile) : hFile(_hFile)
	{
	}

	operator HANDLE()
	{
		return hFile;
	}

	~FileHandle()
	{
		if (hFile != INVALID_HANDLE_VALUE) CloseHandle(hFile);
	}
};

class Utilities :
	public IUtilitiesSupervisor,
	public IMathsVenue,
	public IObserver,
	public Browser::IBrowserFileChangeNotification,
	public IContextMenuEvents
{
	IO::IInstallation& installation;
	IRenderer& renderer;
	AutoFree<Graphics::ITextTesselatorSupervisor> textTesselator;
	Platform* platform = nullptr;
	AutoFree<Graphics::IHQFontsSupervisor> hqFonts;
public:
	Utilities(IO::IInstallation& _installation, IRenderer& _renderer) : installation(_installation), renderer(_renderer)
	{
	}

	~Utilities()
	{
		platform->plumbing.publisher.Unsubscribe(this);
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
		hqFonts = Graphics::CreateHQFonts(platform.graphics.renderer.GuiResources().HQFontsResources());
		platform.plumbing.publisher.Subscribe(this, evUIInvoke);
	}

	void ShowBusy(bool enable, cstr title, cstr messageFormat, ...)
	{
		Rococo::Events::BusyEvent busy;
		busy.isNowBusy = enable;
		busy.message = title;

		va_list args;
		va_start(args, messageFormat);
		SafeVFormat(busy.pingPath.buf, busy.pingPath.CAPACITY, messageFormat, args);
		platform->plumbing.publisher.Publish(busy, Rococo::Events::evBusy);
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

	GUI::IScrollbar* CreateScrollbar(bool _isVertical) override;

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

	IBloodyPropertySetEditorSupervisor* CreateBloodyPropertySetEditor(IEventCallback<BloodyNotifyArgs>& onDirty, IScriptCompilationEventHandler& onCompileUIPanel) override
	{
		return Rococo::CreateBloodyPropertySetEditor(*platform, onDirty, onCompileUIPanel);
	}

	void AddSubtitle(cstr subtitle)
	{
		char fullTitle[256];

		if (subtitle && subtitle[0])
		{
			SafeFormat(fullTitle, sizeof(fullTitle), "%s - %s", platform->os.title, subtitle);
		}
		else
		{
			SafeFormat(fullTitle, sizeof(fullTitle), "%s", platform->os.title);
		}

		SetWindowTextA(platform->os.mainWindow, fullTitle);
	}

	bool GetLoadLocation(Windows::IWindow& parent, LoadDesc& ld) override
	{
		wchar_t filter[128];
		SecureFormat(filter, 128, L"%hs%c%hs%c%c", ld.extDesc, 0, ld.ext, 0, 0);

		OPENFILENAMEW dialog = { 0 };
		dialog.lStructSize = sizeof(dialog);
		dialog.hwndOwner = parent;
		dialog.lpstrFilter = filter;
		dialog.nFilterIndex = 1;
		dialog.lpstrFile = ld.wPath.buf;
		dialog.nMaxFile = Rococo::IO::MAX_PATHLEN;

		wchar_t u16Caption[256];
		SafeFormat(u16Caption, 256, L"%hs", ld.caption);

		dialog.lpstrTitle = u16Caption;
		dialog.Flags = OFN_CREATEPROMPT | OFN_ENABLESIZING | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;

		wchar_t u16Ext[64];
		SafeFormat(u16Caption, 64, L"%hs", ld.ext);

		dialog.lpstrDefExt = u16Ext;

		renderer.SwitchToWindowMode();

		if (GetOpenFileNameW(&dialog))
		{
			ld.shortName = ld.wPath + dialog.nFileOffset;
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
		SecureFormat(filter, 128, L"%hs%c%hs%c%c", sd.extDesc, 0, sd.ext, 0, 0);

		OPENFILENAMEW dialog = { 0 };
		dialog.lStructSize = sizeof(dialog);
		dialog.hwndOwner = parent;
		dialog.lpstrFilter = filter;
		dialog.nFilterIndex = 1;
		dialog.lpstrFile = sd.wPath.buf;
		dialog.nMaxFile = sizeof(sd.wPath) / sizeof(wchar_t);

		wchar_t u16Caption[256];
		SafeFormat(u16Caption, 256, L"%hs", sd.caption);
		dialog.lpstrTitle = u16Caption;

		wchar_t u16Ext[256];
		SafeFormat(u16Ext, 256, L"%hs", sd.ext);
		dialog.lpstrDefExt = u16Ext;

		wchar_t initialPath[IO::MAX_PATHLEN];

		size_t len = wcslen(sd.wPath);

		if (sd.wPath[len - 1] == '\\')
		{
			SafeFormat(initialPath, IO::MAX_PATHLEN, L"%s", sd.wPath.buf);
			sd.wPath.buf[0] = 0;
		}

		renderer.SwitchToWindowMode();

		if (GetSaveFileNameW(&dialog))
		{
			sd.shortName = sd.wPath + dialog.nFileOffset;
			return true;
		}
		else
		{
			int error = CommDlgExtendedError();
			if (error != 0) Throw(error, "Error GetSaveFileNameA");
			return false;
		}
	}

	void EnumerateFiles(IEventCallback<crwstr>& cb, cstr pingPathDirectory) override
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

		WideFilePath shortDir;
		WideFilePath directory;

		Assign(shortDir, pingPathDirectory);

		EndDirectoryWithSlash(shortDir.buf, IO::MAX_PATHLEN);

		Format(directory, L"%ls%ls", installation.Content(), shortDir + 1);
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
		cstr title = caption == nullptr ? platform->os.title : caption;
		renderer.SwitchToWindowMode();
		return ShowMessageBox(parent, question, title, MB_ICONQUESTION | MB_YESNO) == IDYES;
	}

	struct PerformanceStats
	{
		Time::ticks totalLoadCost = 0;
		Time::ticks totalCompileCost = 0;
		Time::ticks totalExecuteCost = 0;
		int64 moduleCallCount = 0;
	};

	stringmap<PerformanceStats> nameToStats;

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
			Format(nas.name, "%s", (cstr) i.first);
			nas.stats = i.second;
			nameAndStats.push_back(nas);
		}

		std::sort(nameAndStats.begin(), nameAndStats.end(),
			[](const NameAndStats& a, const NameAndStats& b)
			{
				return strcmp(a.name, b.name) < 0;
			}
		);

		for (auto& i : nameAndStats)
		{
			float cyclesPerMs = Time::TickHz() / 1000.0f;
			float loadCost = i.stats.totalLoadCost / cyclesPerMs;
			float compileCost = i.stats.totalCompileCost / cyclesPerMs;
			float executeCost = i.stats.totalExecuteCost / cyclesPerMs;
			visitor.ShowString(i.name, " %4.0fms     %4.0fms     %4.0fms     %d", loadCost, compileCost, executeCost, i.stats.moduleCallCount);
		}
	}

	void RunEnvironmentScript(IScriptEnumerator* implicitIncludes, IScriptCompilationEventHandler& _onScriptEvent, const char* name, bool addPlatform, bool shutdownOnFail, bool trace, Strings::IStringPopulator* onScriptCrash, StringBuilder* declarationBuilder)
	{
		RunEnvironmentScriptWithId(implicitIncludes, _onScriptEvent, 0, name, addPlatform, shutdownOnFail, trace, onScriptCrash, declarationBuilder);
	}

	void RunEnvironmentScriptWithId(IScriptEnumerator* implicitIncludes, IScriptCompilationEventHandler& _onScriptEvent, int32 id, const char* name, bool addPlatform, bool shutdownOnFail, bool trace, Strings::IStringPopulator* onScriptCrash, StringBuilder* declarationBuilder) override
	{
		ScriptPerformanceStats stats = { 0 };

		if (name == nullptr)
		{
			Throw(0, "%s: <name> was null", __ROCOCO_FUNCTION__);
		}

		Rococo::MPlatImpl::RunEnvironmentScriptImpl(stats, *platform, implicitIncludes, _onScriptEvent, name, addPlatform, shutdownOnFail, trace, id, onScriptCrash, declarationBuilder);

		auto i = nameToStats.find(name);
		if (i == nameToStats.end())
		{
			i = nameToStats.insert(name, PerformanceStats{}).first;
		}

		i->second.totalCompileCost += stats.compileTime;
		i->second.totalExecuteCost += stats.executeTime;
		i->second.totalLoadCost += stats.loadTime;
		i->second.moduleCallCount = i->second.moduleCallCount + 1;
	}

	void ShowErrorBox(Windows::IWindow& parent, IException& ex, cstr message) override
	{
		renderer.SwitchToWindowMode();
		Windows::ShowErrorBox(parent, ex, message);
	}

	void RefreshResource(cstr pingPath) override
	{
		FileUpdatedEvent fileUpdated;
		fileUpdated.pingPath = pingPath;

		platform->scripts.sourceCache.Release(pingPath);

		platform->plumbing.publisher.Publish(fileUpdated, evFileUpdated);
	}

	IVariableEditor* CreateVariableEditor(Windows::IWindow& window, const Vec2i& span, int32 labelWidth, cstr appQueryName, cstr defaultTab, cstr defaultTooltip, IVariableEditorEventHandler* eventHandler, const Vec2i* topLeft) override
	{
		renderer.SwitchToWindowMode();
		return Rococo::CreateVariableEditor(window, span, labelWidth, appQueryName, defaultTab, defaultTooltip, eventHandler, topLeft);
	}

	void SaveBinary(crwstr pathname, const void* buffer, size_t nChars) override
	{
		FileHandle fh = CreateFileW(pathname, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (fh == INVALID_HANDLE_VALUE)
		{
			Throw(GetLastError(), "Error saving %ls", pathname);
		}

		if (!WriteFile(fh, buffer, (DWORD)nChars, nullptr, nullptr))
		{
			Throw(GetLastError(), "Error writing %ls", pathname);
		}
	}

	AutoFree<IMPlatFileBrowser> browser;
	AutoFree<IPaneBuilderSupervisor> browsingPane;

	void BrowseFiles(IBrowserRulesFactory& factory, IScriptCompilationEventHandler* onCompile)  override
	{
		if (!browser)
		{
			browser = CreateMPlatFileBrowser(platform->plumbing.publisher, platform->os.installation, platform->graphics.gui, platform->hardware.keyboard, *this);
		}

		if (!browsingPane)
		{
			browsingPane = platform->graphics.gui.BindPanelToScript(factory.GetPanePingPath(), onCompile, nullptr);
		}

		browser->Engage(factory);

		platform->graphics.gui.PushTop(browsingPane->Supervisor(), true);
	}

	void OnFileSelect(const U32FilePath& path, bool doubleClick)
	{
		UNUSED(path);
		if (doubleClick)
		{
			OnBrowserSelect();
		}
	}

	void OnBrowserSelect()
	{
		auto* old = platform->graphics.gui.Pop();

		// Assume the top is a file browser
		if (browser->Select())
		{
			browser = nullptr;
			browsingPane = nullptr;
		}
		else
		{
			platform->graphics.gui.PushTop(old, true);
		}
	}

	void OnBrowserCancel()
	{
		// Assume the top is a file browser
		platform->graphics.gui.Pop();
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
			contextMenu = MPlatImpl::CreateContextMenu(platform->plumbing.publisher, *this);
		}

		return *contextMenu;
	}

	void CloseContextMenu()
	{
		if (platform->graphics.gui.Top() == contextMenuPane->Supervisor())
		{
			platform->graphics.gui.Pop();
		}
		else
		{
			Throw(0, "Expecting context menu to be the top level of the gui stack");
		}
	}

	/* IContextMenuEvent::OnItemSelected */
	void OnItemSelected(IContextMenuSupervisor&) override
	{
		CloseContextMenu();
	}

	/* IContextMenuEvent::OnClickOutsideControls */
	void OnClickOutsideControls(IContextMenuSupervisor&) override
	{
		CloseContextMenu();
	}

	IContextMenu& PopupContextMenu(IScriptCompilationEventHandler& onCompile)
	{
		IContextMenuSupervisor& cm = GetContextMenu();

		if (!contextMenuPane)
		{
			contextMenuPane = platform->graphics.gui.BindPanelToScript("!scripts/panels/panel.context-menu.sxy", &onCompile, nullptr);
		}

		platform->graphics.gui.PushTop(contextMenuPane->Supervisor(), true);

		return cm;
	}
};

namespace Rococo
{
	namespace MPlatImpl
	{
		IUtilitiesSupervisor* CreateUtilities(IO::IInstallation& installation, IRenderer& renderer)
		{
			return new Utilities(installation, renderer);
		}
	}
}

GUI::IScrollbar* Utilities::CreateScrollbar(bool _isVertical)
{
	return Rococo::MPlatImpl::CreateScrollbar(_isVertical);
}