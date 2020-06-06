#include "rococo.mplat.h"

#include <rococo.os.win32.h>
#include <rococo.window.h>

#include <Commdlg.h>

#include <vector>
#include <string>
#include <unordered_map>
#include <rococo.variable.editor.h>

#include <algorithm>

#define ROCOCO_USE_SAFE_V_FORMAT
#include <rococo.strings.h>

using namespace Rococo;

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
	namespace M
	{
		void RunEnvironmentScript(ScriptPerformanceStats& stats, Platform& platform, IEventCallback<ScriptCompileArgs>& _onScriptEvent, const char* name, bool addPlatform, bool shutdownOnFail, bool trace);
	}
}

class Utilities : public IUtilitiesSupervisor, public IMathsVenue
{
	IInstallation& installation;
	IRenderer& renderer;
	AutoFree<Graphics::ITextTesselatorSupervisor> textTesselator;
	Platform* platform = nullptr;
public:
	Utilities(IInstallation& _installation, IRenderer& _renderer) : installation(_installation), renderer(_renderer)
	{
	}

	void Free() override
	{
		delete this;
	}

	void SetPlatform(Platform& platform) override
	{
		this->platform = &platform;
		textTesselator = Graphics::CreateTextTesselator(platform);
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

	IBloodyPropertySetEditorSupervisor* CreateBloodyPropertySetEditor(IEventCallback<IBloodyPropertySetEditorSupervisor>& _onDirty) override
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
		SecureFormat(filter, sizeof(filter), L"%S%c%S%c%c", ld.extDesc, 0, ld.ext, 0, 0);

		OPENFILENAMEW dialog = { 0 };
		dialog.lStructSize = sizeof(dialog);
		dialog.hwndOwner = parent;
		dialog.lpstrFilter = filter;
		dialog.nFilterIndex = 1;
		dialog.lpstrFile = ld.path;
		dialog.nMaxFile = sizeof(ld.path);

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
		SecureFormat(filter, sizeof(filter), L"%S%c%S%c%c", sd.extDesc, 0, sd.ext, 0, 0);

		OPENFILENAMEW dialog = { 0 };
		dialog.lStructSize = sizeof(dialog);
		dialog.hwndOwner = parent;
		dialog.lpstrFilter = filter;
		dialog.nFilterIndex = 1;
		dialog.lpstrFile = sd.path;
		dialog.nMaxFile = sizeof(sd.path);

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
			dialog.Flags = OFN_ENABLESIZING;
		}
		else
		{
			dialog.Flags = OFN_ENABLESIZING;
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
		struct : IEventCallback<const wchar_t*>
		{
			std::vector<std::wstring> allResults;
			virtual void OnEvent(const wchar_t* filename)
			{
				allResults.push_back(filename);
			}
		} onFileFound;

		if (pingPathDirectory == nullptr || pingPathDirectory[0] != '!')
		{
			Throw(0, "Directories must be inside the content directory. Use the '!<directory>' notation");
		}

		wchar_t shortDir[IO::MAX_PATHLEN];
		wchar_t directory[IO::MAX_PATHLEN];

		SafeFormat(shortDir, IO::MAX_PATHLEN, L"%S", pingPathDirectory);

		EndDirectoryWithSlash(shortDir, IO::MAX_PATHLEN);

		SafeFormat(directory, IO::MAX_PATHLEN, L"%s%s", (cstr)installation.Content(), shortDir + 1);
		IO::ForEachFileInDirectory(directory, onFileFound);

		std::sort(onFileFound.allResults.begin(), onFileFound.allResults.end());

		for (auto& s : onFileFound.allResults)
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

	void ShowVenue(IMathsVisitor& visitor) override
	{
		visitor.ShowString("", "Loading  Compiling  Execution Call-count");

		for (auto i : nameToStats)
		{
			float cyclesPerMs = OS::CpuHz() / 1000.0f;
			float loadCost = i.second.totalLoadCost / cyclesPerMs;
			float compileCost = i.second.totalCompileCost / cyclesPerMs;
			float executeCost = i.second.totalExecuteCost / cyclesPerMs;
			visitor.ShowString(i.first.c_str(), " %4.0fms     %4.0fms     %4.0fms     %d", loadCost, compileCost, executeCost, i.second.moduleCallCount);
		}
	}

	void RunEnvironmentScript(IEventCallback<ScriptCompileArgs>& _onScriptEvent, const char* name, bool addPlatform, bool shutdownOnFail, bool trace) override
	{
		ScriptPerformanceStats stats = { 0 };
		Rococo::M::RunEnvironmentScript(stats, *platform, _onScriptEvent, name, addPlatform, shutdownOnFail, trace);

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