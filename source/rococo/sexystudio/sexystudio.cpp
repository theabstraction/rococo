// sexystudio.cpp : Defines the entry point for the application.
// Copyright (c) 2021 Mark Anthony Taylor

#include "sexystudio.impl.h"
#include "resource.h"
#include "rococo.auto-release.h"
#include <rococo.io.h>
#include <rococo.events.h>
#include <rococo.strings.h>
#include <uxtheme.h>
#include <rococo.os.h>
#include <shobjidl.h>
#include <sexy.types.h>
#include <Sexy.S-Parser.h>
#include <malloc.h>
#include <stdio.h>
#include <list>
#include <vector>
#include <string>
#include <unordered_map>
#include <rococo.sexystudio.api.h>
#include <algorithm>
#include <rococo.strings.h>
#include <rococo.auto-complete.h>
#include <rococo.sxytype-inference.h>
#include <rococo.auto-complete.h>
#include <rococo.types.inference.h>
#include <rococo.sexml.h>
#include <rococo.debugging.h>
#include <rococo.functional.h>
#include <rococo.api.qualifiers.h>
#include "sexystudio.theme.h"
#include "../sexystudio-lib/sexystudio.database.h"

#pragma comment(lib, "uxtheme.lib")
#pragma comment(lib, "ComCtl32.lib")

using namespace Rococo;
using namespace Rococo::SexyStudio;
using namespace Rococo::Events;
using namespace Rococo::Sex;
using namespace Rococo::Strings;
using namespace Rococo::Windows;

auto evClose = "EvMainIDEWindowCloseRequest"_event;
auto evConfigPathChange = "EvConfigPathChange"_event; // TEventArg<cstr>
auto evProjectChange = "EvProjectChange"_event; // TEventArg<cstr>
auto evSearchChange = "EvSearchChange"_event; // TEventArg<cstr>
auto evMoveAtSearch = "EvMoveAtSearch"_event; // TEventArgs<Vec2i>
auto evDoubleClickSearchList = "EvDoubleClickSearchList"_event; // TEventArgs<cstr>
auto evSearchSelected = "EvSearchSelected"_event; // TEventArg<cstr>
auto evIDEClose = "EvIDEClose"_event;
auto evIDEMin = "EvIDEMin"_event;
auto evIDEMax = "EvIDEMax"_event;
auto evMetaUpdated = "sexystudio.meta.updated"_event;

namespace Rococo::SexyStudio
{
	void PopulateTreeWithSXYFiles(IGuiTree& tree, ISexyDatabase& database, IDBProgress& progress, ISourceTree& sourceTree);

	void PopulateBranchWithSTree(IGuiTree& tree, ISParserTree& srcTree)
	{
		auto branchId = tree.AppendItem(0);
		tree.SetItemText(branchId, srcTree.Source().Name());
	}

	void PopulateTreeWithSXYFiles_Protected(IGuiTree& tree, ISexyDatabase& database, IDBProgress& progress, ISourceTree& sourceTree, ID_TREE_ITEM hRoot)
	{
		struct FileCounterWithProgress : IEventCallback<IO::FileItemData>
		{
			float count = 0;
			IDBProgress* progress;

			IEventCallback<IO::FileItemData>& Advance()
			{
				return *this;
			}

			void OnEvent(IO::FileItemData& item) override
			{
				if (item.isDirectory)
				{
					char progressText[1024];
					SafeFormat(progressText, "Evaluating directory...\r\n   %ls", item.fullPath);
					progress->SetProgress(0.0f, progressText);
				}
				count += 1.0f;
			}
		} fileCounter;

		fileCounter.progress = &progress;


		for (size_t i = 0;; i++)
		{
			auto atom = database.Config().GetSearchPath(i);
			if (!atom.pingPath)
			{
				break;
			}

			if (!atom.isActive)
			{
				continue;
			}

			U8FilePath sysPath;
			database.PingPathResolver().PingPathToSysPath(atom.pingPath, sysPath);

			if (Rococo::IO::IsDirectory(sysPath))
			{
				WideFilePath wPath;
				Assign(wPath, sysPath);
				Rococo::IO::ForEachFileInDirectory(wPath, fileCounter.Advance(), true, nullptr);
			}
		}

		struct DatabaseViewBuilder : IEventCallback<IO::FileItemData>
		{
			IGuiTree* tree;
			ISexyDatabase* database;
			IDBProgress* progress;
			ISourceTree* sourceTree;
			float totalCount = 0;
			float count = 0;

			IEventCallback<IO::FileItemData>& AppendFile()
			{
				return *this;
			}

			void OnEvent(IO::FileItemData& item) override
			{
				count += 1.0f;

				auto idItem = tree->AppendItem((ID_TREE_ITEM)item.containerContext);

				U8FilePath itemText;
				Format(itemText, "%ls", item.itemRelContainer);
				tree->SetItemText(idItem, itemText);

				if (item.isDirectory)
				{
					tree->SetItemImage(idItem, 0);
					tree->SetItemExpandedImage(idItem, 1);

					char progressText[1024];
					SafeFormat(progressText, "Scanning directory...\r\n  %ls", item.fullPath);

					float percent = clamp(totalCount == 0 ? 50.0f : 100.0f * count / totalCount, 0.0f, 99.9f);
					progress->SetProgress(percent, progressText);
				}
				else
				{
					if (EndsWith(item.fullPath, L".sxy"))
					{
						U8FilePath u8Path;
						Format(u8Path, "%ls", item.fullPath);

						char progressText[1024];
						SafeFormat(progressText, "Parsing SXY file...\r\n  %ls", item.fullPath);

						float percent = clamp(totalCount == 0 ? 50.0f : 100.0f * count / totalCount, 0.0f, 99.9f);
						progress->SetProgress(percent, progressText);

						database->UpdateFile_SXY(u8Path);

						tree->SetContext(idItem, 0);

						tree->SetItemImage(idItem, 2);

						sourceTree->Add(idItem, u8Path, 1);
					}
					else
					{
						tree->SetItemImage(idItem, 3);
					}
				}

				item.outContext = (void*)idItem;
			}
		} databaseViewBuilder;

		databaseViewBuilder.tree = &tree;
		databaseViewBuilder.database = &database;
		databaseViewBuilder.progress = &progress;
		databaseViewBuilder.sourceTree = &sourceTree;
		databaseViewBuilder.totalCount = fileCounter.count;

		for (size_t i = 0;; i++)
		{
			auto atom = database.Config().GetSearchPath(i);
			if (!atom.pingPath)
			{
				break;
			}

			if (!atom.isActive)
			{
				continue;
			}

			U8FilePath sysPath;
			database.PingPathResolver().PingPathToSysPath(atom.pingPath, sysPath);

			auto hSearchPath = tree.AppendItem(hRoot);
			tree.SetItemText(hSearchPath, atom.pingPath);

			if (Rococo::IO::IsDirectory(sysPath))
			{
				WideFilePath wPath;
				Assign(wPath, sysPath);
				Rococo::IO::ForEachFileInDirectory(wPath, databaseViewBuilder.AppendFile(), true, (void*)hSearchPath);
			}
			else
			{
				char msg[256];
				SafeFormat(msg, "Could not find %s", sysPath.buf);
				tree.SetItemText(hSearchPath, msg);
			}
		}
	}

	void PopulateTreeWithSXYFiles(IGuiTree& tree, ISexyDatabase& database, IDBProgress& progress, ISourceTree& sourceTree)
	{
		tree.Clear();
		database.Clear();

		WideFilePath scriptPath;
		Format(scriptPath, L"%hs", database.Solution().GetScriptFolder());

		auto hRoot = tree.AppendItem(0);
		tree.SetItemText(hRoot, "!scripts/");

		try
		{
			PopulateTreeWithSXYFiles_Protected(tree, database, progress, sourceTree, hRoot);
		}
		catch (IException& ex)
		{
			auto hError = tree.AppendItem(hRoot);
			tree.SetItemText(hError, "Error recursing search paths");
			auto hErrorMsg = tree.AppendItem(hError);
			tree.SetItemText(hErrorMsg, ex.Message());
			return;
		}

		database.Sort();
	}

}

void ValidateMemory()
{
	if (!_CrtCheckMemory())
	{
		Rococo::OS::TripDebugger();
	}
}

void OpenSexyFile(ISexyStudioEventHandler& evHandler, ISolution& solution, IWindow& mainWindow, cstr path, int lineNumber)
{
	char caption[256];
	HWND hRoot = GetAncestor(mainWindow, GA_ROOT);
	GetWindowTextA(hRoot ? hRoot : mainWindow, caption, sizeof caption);

	try
	{
		auto packagePrefix = "[package]:"_fstring;

		if (StartsWith(path, packagePrefix))
		{
			cstr packagePath = path + packagePrefix.length;
			
			U8FilePath packageFilepath;
			CopyString(packageFilepath.buf, U8FilePath::CAPACITY, solution.GetPackageRoot());

			cstr srcFile = solution.GetPackageSourceFolder(packagePath);
			if (!srcFile)
			{
				Throw(0, "Cannot find package source code for %s. Use map-prefix-to-source in the solution file to specify the location of the package source code", packagePath);
			}

			StringCat(packageFilepath.buf, srcFile, U8FilePath::CAPACITY);

			StringCat(packageFilepath.buf, packagePath, U8FilePath::CAPACITY);
			ReplaceChar(packageFilepath.buf, U8FilePath::CAPACITY, '/', '\\');

			if (!evHandler.TryOpenEditor(packageFilepath, lineNumber))
			{
				Rococo::OS::ShellOpenDocument(mainWindow, caption, packageFilepath, lineNumber);
			}
		}
		else
		{
			if (!evHandler.TryOpenEditor(path, lineNumber))
			{
				Rococo::OS::ShellOpenDocument(mainWindow, caption, path, lineNumber);
			}
		}
	}
	catch (IException& ex)
	{
		Rococo::Windows::ShowErrorBox(mainWindow, ex, "SexyStudio - Error");
	}
}

using namespace Rococo::Sex::SEXML;

struct SearchPathDesc
{
	HString pingPath;
	bool isActive;
};

struct DefaulConfig: IFactoryConfig
{
	int searchPathHeight = 240;
	int packageViewFontHeight = -13;
	int searchViewFontHeight = -13;

	DefaulConfig()
	{
	}

	crwstr ConfigPath() const
	{
		return nullptr;
	}

	SearchPathDescAtom GetSearchPath(size_t index) const override
	{
		UNUSED(index);
		return { nullptr, false };
	}

	cstr GetProjectPath() const override
	{
		return "";
	}

	void SetSearchPathActivity(size_t index, bool isActive) override
	{
		UNUSED(index);
		UNUSED(isActive);
	}

	cstr GetPackage(size_t index) const override
	{
		UNUSED(index);
		return nullptr;
	}

	void CreateLayoutFile()
	{
		Rococo::OS::SaveUserSEXML(nullptr, "sexystudio.layout", true,
			[this](Rococo::Sex::SEXML::ISEXMLBuilder& sb)
			{
				sb.AddDirective("PropertySheets");
				sb.AddAtomicAttribute("searchpath.height", searchPathHeight);
				sb.AddAtomicAttribute("packageview.font_height", packageViewFontHeight);
				sb.AddAtomicAttribute("searchview.font_height", packageViewFontHeight);
				sb.CloseDirective(); // PropertySheets
			}
		);
	}

	void Load()
	{
		try
		{
			if (!Rococo::OS::IsUserSEXMLExistant(nullptr, "sexystudio.layout"))
			{
				// Create it
				CreateLayoutFile();
			}
		}
		catch (IException& ex)
		{
			Rococo::Debugging::AddCriticalLog(ex.Message());
			OS::TripDebugger();
		}

		try
		{
			Rococo::OS::LoadUserSEXML(nullptr, "sexystudio.layout",
				[this](const Rococo::Sex::SEXML::ISEXMLDirectiveList& topLevelDirectives)
				{
					size_t startIndex = 0;
					auto& sheets = GetDirective(topLevelDirectives, "PropertySheets", IN OUT startIndex);
					searchPathHeight = AsAtomicInt32(sheets["searchpath.height"]);
					packageViewFontHeight = AsAtomicInt32(sheets["packageview.font_height"]);
					searchViewFontHeight = AsAtomicInt32(sheets["searchview.font_height"]);
				}
			);
		}
		catch (IException& ex)
		{
			U8FilePath sexmlLayout;
			Rococo::OS::GetUserSEXMLFullPath(sexmlLayout, nullptr, "sexystudio.layout");

			try
			{
				Throw(ex.ErrorCode(), "Error loading %s\nA new layout file will be created.\nError: %s", sexmlLayout.buf, ex.Message());
			}
			catch (IException& inner)
			{
				Windows::ShowErrorBox(Windows::NoParent(), inner, "SexyStudio Error");

				try
				{
					CreateLayoutFile();
				}
				catch (IException& layoutEx)
				{
					OS::TripDebugger();
					Rococo::Debugging::AddCriticalLog(layoutEx.Message());
				}
			}
		}
	}

	void Save()
	{
	}
};

ROCOCO_INTERFACE ISourceChangedEventHandler
{
	virtual void OnPackageSelectionChanged() = 0;
	virtual void OnSearchPathsChanged() = 0;
};

struct PackageEventHandler : IReportWidgetEvent
{
	ISourceChangedEventHandler& eventHandler;

	void OnItemLeftClicked(int index, int subItem, IReportWidget& source) override
	{
		int image = source.GetImageIndex(index, subItem);
		source.SetImageIndex(index, subItem, image == 0 ? 1 : 0);	
		eventHandler.OnPackageSelectionChanged();
	}

	void OnItemRightClicked(int index, int subItem, IReportWidget& source) override
	{
		UNUSED(index);
		UNUSED(subItem);
		UNUSED(source);
	}

	PackageEventHandler(ISourceChangedEventHandler& _eventHandler): eventHandler(_eventHandler)
	{

	}
};

struct SearchPathEventHandler : IReportWidgetEvent
{
	ISourceChangedEventHandler& eventHandler;

	void OnItemLeftClicked(int index, int subItem, IReportWidget& source) override
	{
		int image = source.GetImageIndex(index, subItem);
		source.SetImageIndex(index, subItem, image == 0 ? 1 : 0);
		eventHandler.OnSearchPathsChanged();
	}

	void OnItemRightClicked(int index, int subItem, IReportWidget& source) override
	{
		UNUSED(index);
		UNUSED(subItem);
		UNUSED(source);
	}

	SearchPathEventHandler(ISourceChangedEventHandler& _eventHandler) : eventHandler(_eventHandler)
	{
	}
};

class PropertySheets: IObserver, IGuiTreeRenderer, IGuiTreeEvents, ISourceChangedEventHandler
{
private:
	WidgetContext wc;
	DefaulConfig& config;
	IIDEFrame& ideFrame;
	ISexyDatabaseSet& databaseSet;
	AutoFree<ISourceTree> idToSourceMap = CreateSourceTree();
	IGuiTree* fileBrowser = nullptr;
	ITab* projectTab = nullptr;
	U8FilePath configDirectory;
	U8FilePath projectPath;
	IFilePathEditor* projectDirEditor;
	IReportWidget* searchView;
	IReportWidget* packageView;
	PackageEventHandler packageViewEventHandler;
	SearchPathEventHandler searchPathEventHandler;

	void SetProjectDirectory()
	{
		if (*projectPath.buf == '!')
		{
			try
			{
				projectDirEditor->UpdateText();
				SyncContent();
			}
			catch (...)
			{
				// Bad conversion
			}
		}
	}

	void SyncContent()
	{
		WaitCursorSection waitSection;
		ideFrame.SetProgress(0.0f, "Populating file browser...");

		if (!EndsWith(configDirectory, "\\"))
		{
			StringCat(configDirectory.buf, "\\", U8FilePath::CAPACITY);
		}

		try
		{
			databaseSet.SetConfigDirectory(configDirectory);
		}
		catch (IException& ex)
		{
			ShowErrorBox(ideFrame.Window(), ex, "Error loading config: %s", configDirectory);
			ideFrame.SetProgress(100.0f, "Populated file browser");
			return;
		}

		PopulateTreeWithSXYFiles(*fileBrowser, databaseSet.GetDatabase(), ideFrame, *idToSourceMap);
		ideFrame.SetProgress(100.0f, "Populated file browser");

		int nPackages = packageView->GetNumberOfRows();
		for (int i = 0; i < nPackages; i++)
		{
			if (1 == packageView->GetImageIndex(i, 0))
			{
				U8FilePath pingPath;
				packageView->GetText(pingPath, i, 0);
				if (pingPath.buf[0] == '!')
				{
					U8FilePath sysPath;
					databaseSet.GetDatabase().PingPathResolver().PingPathToSysPath(pingPath, sysPath);
					PopulateTreeWithPackage(sysPath, databaseSet.GetDatabase());
				}
			}
		}

		databaseSet.GetDatabase().Sort();

		searchView->ClearItems();

		for (size_t index = 0;; index++)
		{
			auto atom = databaseSet.GetDatabase().Config().GetSearchPath(index);
			if (atom.pingPath == nullptr)
			{
				break;
			}

			searchView->SetItem("Path", atom.pingPath, -1, atom.isActive ? 1 : 0);
		}

		try
		{
			U8FilePath sysPackagePath;
			databaseSet.GetDatabase().PingPathResolver().PingPathToSysPath("!packages", sysPackagePath);

			PopulatePackageViewWithCheckboxes(sysPackagePath);
		}
		catch (...)
		{

		}

		TEventArgs<ISexyDatabase*> args;
		args.value = &databaseSet.GetDatabase();
		wc.publisher.Publish(args, evMetaUpdated);
	}

	void PopulatePackageViewWithCheckboxes(cstr packagePath)
	{
		AutoFree<IO::IPathCacheSupervisor> pathCache = IO::CreatePathCache();

		try
		{
			pathCache->AddLegalExtension(".sxyz");
			pathCache->AddPathsFromDirectory(packagePath, false);
			pathCache->Sort();
		}
		catch (IException& ex)
		{
		//	Rococo::Debugging::AddCriticalLog("Suggestion: check that the content/packages folder exists and that it is in good order");
			Windows::ShowErrorBox(ideFrame, ex, "SexyStudio package reading issue");
		}

		packageView->ClearItems();

		for (size_t i = 0; i < pathCache->NumberOfFiles(); ++i)
		{
			cstr path = pathCache->GetFileName(i);

			U8FilePath pingPath;
			databaseSet.GetDatabase().PingPathResolver().SysPathToPingPath(path, pingPath);

			bool isASelectedPackage = false;

			int j = 0;
			for (;;)
			{
				cstr p = databaseSet.GetDatabase().Config().GetPackage(j++);
				if (!p)
				{
					break;
				}
			
				if (Eq(p, pingPath))
				{
					isASelectedPackage = true;
					break;
				}
			}

			packageView->SetItem("Package", pingPath, -1, isASelectedPackage ? 1 : 0);
		}
	}

	void OnEvent(Event& ev) override
	{
		if (ev == evConfigPathChange)
		{
			SyncContent();
		}
		else if (ev == evProjectChange)
		{
			SetProjectDirectory();
		}
	}

	void RenderItem() override
	{

	}

	void OnPackageSelectionChanged()
	{
		SyncContent();
	}

	void OnSearchPathsChanged()
	{
		for (int row = 0; row < searchView->GetNumberOfRows(); row++)
		{
			U8FilePath pingPath;
			searchView->GetText(pingPath, row, 0);

			bool isActive = searchView->GetImageIndex(row, 0);

			for (size_t index = 0;;index++)
			{
				auto atom = databaseSet.GetDatabase().Config().GetSearchPath(index);
				if (atom.pingPath == nullptr)
				{
					break;
				}

				if (Eq(atom.pingPath, pingPath))
				{
					databaseSet.GetDatabase().Config().SetSearchPathActivity(index, isActive);
					break;
				}
			}
		}
		SyncContent();
	}
public:
	PropertySheets(DefaulConfig& _config, ISplitScreen& screen, IIDEFrame& _ideFrame, ISexyDatabaseSet& _databaseSet):
		wc(screen.Children()->Context()),
		config(_config),
		ideFrame(_ideFrame),
		databaseSet(_databaseSet),
		packageViewEventHandler(*this),
		searchPathEventHandler(*this)
	{
		Format(configDirectory, "%s", databaseSet.GetDatabase().Solution().GetContentFolder());
		Format(projectPath, "%s", databaseSet.GetDatabase().Config().GetProjectPath());

		screen.SetBackgroundColour(RGBAb(128, 192, 128));

		ITabSplitter* tabs = CreateTabSplitter(*screen.Children());
		tabs->SetVisible(true);

		projectTab = &tabs->AddTab();
		projectTab->SetName("Projects");
		projectTab->SetTooltip("Project View");


		IVariableList* projectSettings = CreateVariableList(projectTab->Children(), databaseSet.GetDatabase().PingPathResolver());
		projectSettings->SetVisible(true);

		Widgets::AnchorToParentTop(*projectSettings, 0);
		Widgets::AnchorToParentLeft(*projectSettings, 0);
		Widgets::AnchorToParentRight(*projectSettings, 0);

		auto* configPathEditor = projectSettings->AddFilePathEditor(EFilePathType::SYS_PATHS);
		configPathEditor->SetName("Config-Directory");

		configPathEditor->Bind(configDirectory, 128);
		configPathEditor->SetVisible(true);
		configPathEditor->SetUpdateEvent(evConfigPathChange);

		projectDirEditor = projectSettings->AddFilePathEditor(EFilePathType::PING_PATHS);
		projectDirEditor->SetName("Project");

		projectDirEditor->Bind(projectPath, 128);
		projectDirEditor->SetVisible(true);
		projectDirEditor->SetUpdateEvent(evProjectChange);

		packageView = projectSettings->AddReportWidget(packageViewEventHandler);
		packageView->SetDefaultHeight(128);
		packageView->SetFont(config.packageViewFontHeight, "Consolas");
		packageView->AddColumn("Package", "Package", 320);
		packageView->SetVisible(true);

		searchView = projectSettings->AddReportWidget(searchPathEventHandler);
		searchView->SetDefaultHeight(config.searchPathHeight);
		searchView->SetFont(config.searchViewFontHeight, "Consolas");
		searchView->AddColumn("Path", "Search Path", 320);
		searchView->SetVisible(true);

		Widgets::ExpandBottomFromTop(*projectSettings, projectSettings->GetDefaultHeight());

		TreeStyle style;
		style.hasButtons = true;
		style.hasLines = true;

		fileBrowser = CreateTree(projectTab->Children(), style, *this, this);
		Widgets::AnchorToParent(*fileBrowser, 0, projectSettings->GetDefaultHeight(), 0, 0);

		fileBrowser->SetVisible(true);
		fileBrowser->SetImageList(4, IDB_FOLDER_CLOSED, IDB_FOLDER_OPEN, IDB_FILETYPE_SXY, IDB_FILETYPE_UNKNOWN);

		ideFrame.SetProgress(100.0f, "Complete!");

		wc.publisher.Subscribe(this, evConfigPathChange);
		wc.publisher.Subscribe(this, evProjectChange);
	}

	~PropertySheets()
	{
		wc.publisher.Unsubscribe(this);

		for (int i = 0; i < packageView->GetNumberOfRows(); ++i)
		{
			int imageIndex = packageView->GetImageIndex(i, 0);
			if (imageIndex == 1)
			{
				U8FilePath path;
				packageView->GetText(path, i, 0);
				if (*path == '!')
				{
				}
			}
		}
	}

	void SetContent(cstr newPath)
	{
		if (Eq(configDirectory, newPath))
		{
			return;
		}

		Assign(configDirectory, newPath);

		SyncContent();
	}


	void CollapseTree()
	{
		fileBrowser->Collapse();
	}

	ISexyDatabase& Database() { return databaseSet.GetDatabase();  }

	void SelectProjectTab()
	{
		projectTab->Activate();
	}

	enum { COMMAND_OPEN_FILE = 1001, COMMAND_FOCUS_PROJECT };
	HString popupTargetFile;
	int popupTargetFileLine = 0;

	void OnCommand(uint16 id) override
	{
		if (popupTargetFile.length() == 0)
		{
			return;
		}

		if (id == COMMAND_OPEN_FILE)
		{
			OpenSexyFile(ideFrame.Events(), databaseSet.GetDatabase().Solution(), ideFrame.Window(), popupTargetFile.c_str(), popupTargetFileLine);
		}
		else if (id == COMMAND_FOCUS_PROJECT)
		{
			try
			{
				databaseSet.GetDatabase().FocusProject(popupTargetFile);

				databaseSet.GetDatabase().Sort();

				TEventArgs<ISexyDatabase*> args;
				args.value = &databaseSet.GetDatabase();
				wc.publisher.Publish(args, evMetaUpdated);
			}
			catch (IException& ex)
			{
				Rococo::Windows::ShowErrorBox(ideFrame.Window(), ex, "SexyStudio - Error");
			}
		}
	}

	void OnItemContextClick(IGuiTree& tree, ID_TREE_ITEM hItem, Vec2i pos) override
	{
		auto src = idToSourceMap->Find(hItem);
		if (!src.SourcePath)
		{
			return;
		}

		popupTargetFile = src.SourcePath;
		popupTargetFileLine = src.LineNumber;

		auto& popup = tree.PopupMenu();

		popup.ClearPopupMenu();
		popup.AppendMenuItem(COMMAND_OPEN_FILE, "Open");
		popup.AppendMenuItem(COMMAND_FOCUS_PROJECT, "Focus Project");
		popup.ShowPopupMenu(pos);
	}
};


bool IsDotted(cstr name)
{
	cstr s;
	for (s = name; *s != 0; s++)
	{
		if (*s == '.') return true;
	}

	return false;
}

enum class ClassImageIndex: int32
{
	BLANK,
	NAMESPACE, 
	INTERFACE,
	METHOD,
	STRUCT, 
	STRONG,
	FIELD, 
	EXTENDS, 
	ATTRIBUTE, 
	FUNCTION,
	INPUT, 
	OUTPUT,
	ENUM,
	ALIAS,
	FACTORY,
	ARCHETYPE
};

class SexyExplorer: IObserver, IGuiTreeEvents
{
private:
	WidgetContext wc;
	DefaulConfig& config;
	ISplitScreen& screen;
	ISexyDatabaseSet& databaseSet;
	IGuiTree* classTree;
	ITab* classTab = nullptr;
	ISexyStudioEventHandler& eventHandler;

	enum { OPEN_ITEM = 1001 };	
	
	HString popupSourceModule;
	int popupSourceModuleLineNumber = 0;

	void OnCommand(uint16 id) override
	{
		if (id == OPEN_ITEM && popupSourceModule.length() > 0)
		{
			OpenSexyFile(eventHandler, databaseSet.GetDatabase().Solution(), screen.Window(), popupSourceModule, popupSourceModuleLineNumber);
		}
	}

	void OnItemContextClick(IGuiTree& tree, ID_TREE_ITEM hItem, Vec2i pos) override
	{
		popupSourceModule = "";

		auto i = idToFunction.find(hItem);
		if (i != idToFunction.end())
		{
			auto* src = i->second->LocalFunction()->SourcePath();
			if (src)
			{
				popupSourceModule = src;
				popupSourceModuleLineNumber = i->second->LocalFunction()->LineNumber();

				auto& popup = tree.PopupMenu();
				popup.ClearPopupMenu();
				popup.AppendMenuItem(OPEN_ITEM, "Open");
				popup.ShowPopupMenu(pos);
			}
		}

		auto j = idToInterface.find(hItem);
		if (j != idToInterface.end())
		{
			auto* src = j->second->SourcePath();
			if (src)
			{
				popupSourceModule = src;
				popupSourceModuleLineNumber = 1;

				auto& popup = tree.PopupMenu();
				popup.ClearPopupMenu();
				popup.AppendMenuItem(OPEN_ITEM, "Open");
				popup.ShowPopupMenu(pos);
			}
		}

		auto k = idToFactory.find(hItem);
		if (k != idToFactory.end())
		{
			auto* src = k->second->SourcePath();
			if (src)
			{
				popupSourceModule = src;
				popupSourceModuleLineNumber = k->second->LineNumber();

				auto& popup = tree.PopupMenu();
				popup.ClearPopupMenu();
				popup.AppendMenuItem(OPEN_ITEM, "Open");
				popup.ShowPopupMenu(pos);
			}
		}

		auto l = idToType.find(hItem);
		if (l != idToType.end())
		{
			auto* src = l->second->LocalType()->SourcePath();
			if (src)
			{
				popupSourceModule = src;
				popupSourceModuleLineNumber = l->second->LocalType()->LineNumber();

				auto& popup = tree.PopupMenu();
				popup.ClearPopupMenu();
				popup.AppendMenuItem(OPEN_ITEM, "Open");
				popup.ShowPopupMenu(pos);
			}
		}

		auto m = idToArchetype.find(hItem);
		if (m != idToArchetype.end())
		{
			auto* src = m->second->SourcePath();
			if (src)
			{
				popupSourceModule = src;
				popupSourceModuleLineNumber = m->second->LineNumber();

				auto& popup = tree.PopupMenu();
				popup.ClearPopupMenu();
				popup.AppendMenuItem(OPEN_ITEM, "Open");
				popup.ShowPopupMenu(pos);
			}
		}
	}

	void AppendArguments(ID_TREE_ITEM idFunction, ISXYArchetype& archetype)
	{
		if (archetype.InputCount() + archetype.OutputCount() == 0)
		{
			auto idNoArg = classTree->AppendItem(idFunction);
			classTree->SetItemText(idNoArg, " - no arguments -");
			classTree->SetItemImage(idNoArg, (int)ClassImageIndex::BLANK);
		}

		for (int k = 0; k < archetype.InputCount(); ++k)
		{
			auto idInputArg = classTree->AppendItem(idFunction);
			char desc[256];

			cstr qualifier;

			auto inputQualifier = archetype.InputQualifier(k);

			switch (inputQualifier)
			{
			default:
				qualifier = "";
				break;
			case EQualifier::Constant:
				qualifier = "const ";
				break;
			case EQualifier::Output:
				qualifier = "output ";
				break;
			case EQualifier::Ref:
				qualifier = "ref ";
				break;
			}

			SafeFormat(desc, "%s%s %s", qualifier, archetype.InputType(k), archetype.InputName(k));
			classTree->SetItemText(idInputArg, desc);
			classTree->SetItemImage(idInputArg, (int)ClassImageIndex::INPUT);
		}

		if (archetype.OutputCount() > 0)
		{
			auto idMapArg = classTree->AppendItem(idFunction);
			classTree->SetItemText(idMapArg, "->");
			classTree->SetItemImage(idMapArg, (int)ClassImageIndex::BLANK);

			for (int k = 0; k < archetype.OutputCount(); ++k)
			{
				auto idOutputArg = classTree->AppendItem(idFunction);
				char desc[256];
				SafeFormat(desc, "%s %s", archetype.OutputType(k), archetype.OutputName(k));
				classTree->SetItemText(idOutputArg, desc);
				classTree->SetItemImage(idOutputArg, (int)ClassImageIndex::OUTPUT);
			}
		}
	}

	std::unordered_map<ID_TREE_ITEM, ISXYInterface*> idToInterface;

	void AppendInterfaces(ISxyNamespace& ns, ID_TREE_ITEM idNSNode)
	{
		for (int i = 0; i < ns.InterfaceCount(); ++i)
		{
			auto& interf = ns.GetInterface(i);
			auto idInterface = classTree->AppendItem(idNSNode);

			idToInterface[idInterface] = &interf;

			cstr className = nullptr;

			cr_sex sDef = interf.GetDefinition();
			if (Eq(sDef[0].c_str(), "class"))
			{
				if (IsAtomic(sDef[1]))
				{
					className = sDef[1].c_str();
				}
			}

			char desc[256];

			if (className)
			{
				char subdesc[128];
				SafeFormat(subdesc, "%s (defined by %s)", interf.PublicName(), className);
				SafeFormat(desc, "%-64.64s %s", subdesc, interf.SourcePath());
			}
			else
			{
				SafeFormat(desc, "%-64.64s %s", interf.PublicName(), interf.SourcePath());
			}

			classTree->SetItemText(idInterface, desc);
			classTree->SetItemImage(idInterface, (int)ClassImageIndex::INTERFACE);

			cstr base = interf.Base();
			if (base)
			{
				auto idBase = classTree->AppendItem(idInterface);
				SafeFormat(desc, "extends %s", base);
				classTree->SetItemText(idBase, desc);
				classTree->SetItemImage(idBase, (int)ClassImageIndex::EXTENDS);
			}

			for (int j = 0; j < interf.AttributeCount(); ++j)
			{
				cstr attr = interf.GetAttribute(j);
				SafeFormat(desc, "attribute %s", attr);
				auto idAttr = classTree->AppendItem(idInterface);
				classTree->SetItemText(idAttr, desc);
				classTree->SetItemImage(idAttr, (int)ClassImageIndex::ATTRIBUTE);
			}

			for (int j = 0; j < interf.MethodCount(); ++j)
			{
				auto& method = interf.GetMethod(j);

				auto idMethod = classTree->AppendItem(idInterface);
				classTree->SetItemText(idMethod, method.PublicName());
				classTree->SetItemImage(idMethod, (int)ClassImageIndex::METHOD);

				AppendArguments(idMethod, method);
			}
		}
	}

	std::unordered_map<ID_TREE_ITEM, ISXYType*> idToType;

	void AppendTypes(ISxyNamespace& ns, ID_TREE_ITEM idNSNode, ISexyDatabase& database)
	{
		UNUSED(database);
		for (int i = 0; i < ns.TypeCount(); ++i)
		{
			auto& type = ns.GetType(i);
			auto idType = classTree->AppendItem(idNSNode);
			cstr publicName = type.PublicName();
			auto* localType = type.LocalType();
			UNUSED(publicName);

			idToType[idType] = &type;

			char desc[256];

			if (localType && localType->IsStrong())
			{
				char strongdesc[64];
				SafeFormat(strongdesc, "%s (strong %s)", type.PublicName(), localType->GetField(0).type);
				SafeFormat(desc, "%-64.64s %s", strongdesc, localType ? localType->SourcePath() : "");
			}
			else
			{
				SafeFormat(desc, "%-64.64s %s", type.PublicName(), localType ? localType->SourcePath() : "");
			}

			classTree->SetItemText(idType, desc);

			ClassImageIndex index = localType && localType->IsStrong() ? ClassImageIndex::STRONG : ClassImageIndex::STRUCT;

			classTree->SetItemImage(idType, (int)index);

			if (localType)
			{
				if (localType->IsStrong())
				{
					auto idField = classTree->AppendItem(idType);

					auto field = localType->GetField(0);

					SafeFormat(desc, "%s %s", field.type, field.name);
					classTree->SetItemText(idField, desc);
					classTree->SetItemImage(idField, (int)ClassImageIndex::FIELD);
				}
				else
				{
					for (int j = 0; j < localType->FieldCount(); ++j)
					{
						auto idField = classTree->AppendItem(idType);

						auto field = localType->GetField(j);

						SafeFormat(desc, "%s %s", field.type, field.name);
						classTree->SetItemText(idField, desc);
						classTree->SetItemImage(idField, (int)ClassImageIndex::FIELD);
					}
				}
			}
		}
	}

	std::unordered_map<ID_TREE_ITEM, ISXYArchetype*> idToArchetype;

	void AppendArchetypes(ISxyNamespace& ns, ID_TREE_ITEM idNSNode, ISexyDatabase& database, bool appendSourceName)
	{
		UNUSED(database);
		for (int i = 0; i < ns.ArchetypeCount(); ++i)
		{
			auto& archetype = ns.GetArchetype(i);
			auto idArchetype = classTree->AppendItem(idNSNode);
			cstr publicName = archetype.PublicName();

			idToArchetype[idArchetype] = &archetype;

			char desc[256];
			SafeFormat(desc, "%-64.64s %s", publicName, appendSourceName ? archetype.SourcePath() : "");
			classTree->SetItemText(idArchetype, desc);
			classTree->SetItemImage(idArchetype, (int)ClassImageIndex::ARCHETYPE);

			AppendArguments(idArchetype, archetype);
		}
	}

	std::unordered_map<ISXYPublicFunction*, ID_TREE_ITEM> mapPublicFunctionToTreeItem;

	void MapFunctionToClassTree(ISXYPublicFunction& function, ID_TREE_ITEM itemId)
	{
		mapPublicFunctionToTreeItem[&function] = itemId;
	}

	std::unordered_map<ID_TREE_ITEM, ISXYPublicFunction*> idToFunction;

	void AppendFunctions(ISxyNamespace& ns, ID_TREE_ITEM idNSNode, ISexyDatabase& database, bool appendSourceName)
	{
		UNUSED(database);
		for (int i = 0; i < ns.FunctionCount(); ++i)
		{
			auto& function = ns.GetFunction(i);
			auto idFunction = classTree->AppendItem(idNSNode);
			cstr publicName = function.PublicName();
			auto* localFunction = function.LocalFunction();

			idToFunction[idFunction] = &function;

			MapFunctionToClassTree(function, idFunction);

			char desc[256];
			SafeFormat(desc, "%-64.64s %s", publicName, appendSourceName && localFunction ? localFunction->SourcePath() : "");
			classTree->SetItemText(idFunction, desc);
			classTree->SetItemImage(idFunction, (int)ClassImageIndex::FUNCTION);

			if (localFunction)
			{
				AppendArguments(idFunction, *localFunction);
			}
		}
	}

	std::unordered_map<ID_TREE_ITEM, ISXYFactory*> idToFactory;

	void AppendFactories(ISxyNamespace& ns, ID_TREE_ITEM idNSNode, ISexyDatabase& database, bool appendSourceName)
	{
		UNUSED(database);
		for (int i = 0; i < ns.FactoryCount(); ++i)
		{
			auto& factory = ns.GetFactory(i);
			auto idFactory = classTree->AppendItem(idNSNode);
			cstr publicName = factory.PublicName();

			idToFactory[idFactory] = &factory;

			char definedInterface[256];
			factory.GetDefinedInterface(definedInterface, sizeof definedInterface);

			char shortDesc[256];
			SafeFormat(shortDesc, "%-24.24s --> (%s)", publicName, definedInterface);

			char desc[256];
			SafeFormat(desc, "%-64.64s %s", shortDesc, appendSourceName  ? factory.SourcePath() : "");
			classTree->SetItemText(idFactory, desc);
			classTree->SetItemImage(idFactory, (int)ClassImageIndex::FACTORY);

			if (factory.InputCount()  == 0)
			{
				auto idNoArg = classTree->AppendItem(idFactory);
				classTree->SetItemText(idNoArg, " - no arguments -");
				classTree->SetItemImage(idNoArg, (int)ClassImageIndex::BLANK);
			}

			for (int k = 0; k < factory.InputCount(); ++k)
			{
				auto idInputArg = classTree->AppendItem(idFactory);
				SafeFormat(desc, "%s %s", factory.InputType(k), factory.InputName(k));
				classTree->SetItemText(idInputArg, desc);
				classTree->SetItemImage(idInputArg, (int)ClassImageIndex::INPUT);
			}
		}
	}

	void AppendEnumerations(ISxyNamespace& ns, ID_TREE_ITEM idNSNode, ISexyDatabase& database)
	{
		UNUSED(database);
		if (ns.EnumCount() > 0)
		{
			auto idEnums = classTree->AppendItem(idNSNode);
			classTree->SetItemText(idEnums, "Enumerations");
			classTree->SetItemImage(idEnums, (int)ClassImageIndex::ENUM);

			for (int i = 0; i < ns.EnumCount(); ++i)
			{
				cstr enumName = ns.GetEnumName(i);
				cstr enumText = ns.GetEnumValue(i);

				auto idEnum = classTree->AppendItem(idEnums);

				char desc[256];
				SafeFormat(desc, "%s = %s", enumName, enumText);

				char fulldesc[256];
				SafeFormat(fulldesc, "%-64.64s %s", desc, ns.GetEnumSourcePath(i));
				classTree->SetItemText(idEnum, fulldesc);
				classTree->SetItemImage(idEnum, (int)ClassImageIndex::ENUM);
			}
		}
	}

	void AppendAliases(ISxyNamespace& ns, ID_TREE_ITEM idNSNode, ISexyDatabase& database)
	{
		UNUSED(database);
		for (int i = 0; i < ns.AliasCount(); ++i)
		{
			cstr from = ns.GetNSAliasFrom(i);
			cstr to = ns.GetNSAliasTo(i);

			auto idAlias = classTree->AppendItem(idNSNode);

			char desc[256];
			SafeFormat(desc, "%s (synonymous with %s)", to, from);

			char fulldesc[256];
			SafeFormat(fulldesc, "%-64.64s %s", desc, ns.GetAliasSourcePath(i));
			classTree->SetItemText(idAlias, fulldesc);
			classTree->SetItemImage(idAlias, (int)ClassImageIndex::ALIAS);
		}
	}

	void AppendNamespaceRecursive(ISxyNamespace& ns, ID_TREE_ITEM idNSNode, ISexyDatabase& database)
	{
		for (int i = 0; i < ns.SubspaceCount(); ++i)
		{
			auto& subspace = ns[i];
			auto idBranch = classTree->AppendItem(idNSNode);
			mapNSToTreeItemId[&subspace] = idBranch;
			mapTreeItemIdToNS[idBranch] = &subspace;
			classTree->SetItemText(idBranch, subspace.Name());
			classTree->SetItemImage(idBranch, (int)ClassImageIndex::NAMESPACE);
			AppendNamespaceRecursive(subspace, idBranch, database);
		}

		AppendEnumerations(ns, idNSNode, database);
		AppendInterfaces(ns, idNSNode);
		AppendFactories(ns, idNSNode, database, true);
		AppendTypes(ns, idNSNode, database);
		AppendFunctions(ns, idNSNode, database, true);
		AppendAliases(ns, idNSNode, database);
		AppendArchetypes(ns, idNSNode, database, true);
	}

	void OnMetaChanged(ISexyDatabase& database)
	{
		classTree->Clear();

		ID_TREE_ITEM idNamespace = classTree->AppendItem(0);
		classTree->SetItemText(idNamespace, "Namespaces");
		classTree->SetItemImage(idNamespace, (int) ClassImageIndex::NAMESPACE);

		mapNSToTreeItemId.clear();
		mapTreeItemIdToNS.clear();
		mapPublicFunctionToTreeItem.clear();

		idToArchetype.clear();
		idToFactory.clear();
		idToFunction.clear();
		idToInterface.clear();
		idToType.clear();

		AppendNamespaceRecursive(database.GetRootNamespace(), idNamespace, database);
	}

	struct SearchItem
	{
		std::string text;
		ISxyNamespace* ns;
		ISXYPublicFunction* function;
	};

	std::vector<SearchItem> searchArrayResults;

	bool isDirty = true;

	std::unordered_map<ID_TREE_ITEM, ISxyNamespace*> mapTreeItemIdToNS;
	std::unordered_map<ISxyNamespace*, ID_TREE_ITEM> mapNSToTreeItemId;

	void AddRootSubspacesToSearchResults()
	{
		auto& root = databaseSet.GetDatabase().GetRootNamespace();

		for (int i = 0; i < root.SubspaceCount(); ++i)
		{
			auto& ns = root[i];
			searchArrayResults.push_back({ ns.Name(), &ns, nullptr });
		}
	}

	void RefreshResultList()
	{
		if (!IsWindowVisible(searchResults->Window()))
		{
			GuiRect rect = Widgets::GetScreenRect(searchEditor->OSEditor());
			rect.top = rect.bottom;
			rect.bottom = rect.top + 120;
			Widgets::SetWidgetPosition(*searchResults, rect);
			searchResults->SetVisible(true);

			SetFocus(searchEditor->OSEditor());
			searchResults->RenderWhileMouseInEditorOrList(searchEditor->OSEditor());
		}

		if (!isDirty)
		{
			return;
		}

		if (searchArrayResults.empty())
		{
			AddRootSubspacesToSearchResults();
		}

		if (searchArrayResults.empty())
		{
			return;
		}

		searchResults->ClearItems();

		for (auto& item : searchArrayResults)
		{
			searchResults->AppendItem(item.text.c_str());
		}

		isDirty = false;
	}

	void EnumerateNamesStartingWith(ISxyNamespace& ns, cstr prefix, Strings::IStringPopulator& cb)
	{
		for (int i = 0; i < ns.SubspaceCount(); ++i)
		{
			auto* name = ns[i].Name();
			if (StartsWith(name, prefix))
			{
				cb.Populate(name);
			}
		}

		for (int j = 0; j < ns.InterfaceCount(); ++j)
		{
			auto* name = ns.GetInterface(j).PublicName();
			if (StartsWith(name, prefix))
			{
				cb.Populate(name);
			}
		}

		for (int j = 0; j < ns.FunctionCount(); ++j)
		{
			auto* name = ns.GetFunction(j).PublicName();
			if (StartsWith(name, prefix))
			{
				cb.Populate(name);
			}
		}

		for (int j = 0; j < ns.FactoryCount(); ++j)
		{
			auto* name = ns.GetFactory(j).PublicName();
			if (StartsWith(name, prefix))
			{
				cb.Populate(name);
			}
		}
	}

	struct FuzzyMatch
	{
		std::string candidate;
		int levenshteinDistance;
		ISxyNamespace* ns;
		ISXYPublicFunction* publicFunction;
	};

	std::vector<FuzzyMatch> fuzzyMatches;

	std::list<char> searchBuffer;
	std::list<char> candidateBuffer;

	// Shyreman's-Amazing-Fuzzy-String-Matching algorithm
	bool AppendFuzzyStringMatch(cstr searchTerm, cstr candidate, ISxyNamespace* ns, ISXYPublicFunction* publicFunction)
	{
		if (Eq(searchTerm, candidate))
		{
			fuzzyMatches.push_back({ candidate, 0, ns, publicFunction });
			return true;
		}

		if (StartsWith(candidate, searchTerm))
		{
			fuzzyMatches.push_back({ candidate, StringLength(candidate) - StringLength(searchTerm), ns, publicFunction });
			return true;
		}

		searchBuffer.clear();
		candidateBuffer.clear();

		for (cstr s = searchTerm; *s != 0; ++s)
		{
			searchBuffer.push_back(*s);
		}

		for (cstr t = candidate; *t != 0; ++t)
		{
			candidateBuffer.push_back(*t);
		}

		bool match = false;
		int score = 0;

		while (!searchBuffer.empty())
		{
			char c = searchBuffer.front();
			searchBuffer.pop_front();

			for (auto i = candidateBuffer.begin(); i != candidateBuffer.end(); ++i)
			{
				if (c == *i)
				{
					match = true;
					candidateBuffer.erase(i);
					break;
				}

				if (toupper(c) == toupper(*i))
				{
					match = true;
					score += 1;
					candidateBuffer.erase(i);
					break;
				}

				score += 2;
			}
		}

		score += 2 * (int) candidateBuffer.size();

		if (match) fuzzyMatches.push_back(FuzzyMatch { candidate, score, ns, publicFunction });

		return match;
	}

	void AppendFuzzyStringMatchSubspacesAndPublicFunctions(cstr searchTerm, ISxyNamespace& ns)
	{
		for (int i = 0; i < ns.SubspaceCount(); ++i)
		{
			auto& subspace = ns[i];
			if (subspace.Name()[0] != 0)
			{
				AppendFuzzyStringMatch(searchTerm, subspace.Name(), &subspace, nullptr);
			}
			AppendFuzzyStringMatchSubspacesAndPublicFunctions(searchTerm, subspace);
		}

		for (int i = 0; i < ns.FunctionCount(); ++i)
		{
			auto& f = ns.GetFunction(i);		
			AppendFuzzyStringMatch(searchTerm, f.PublicName(), &ns, &f);
		}
	}

	void AppendFuzzyItemsToSearchTerms(ISxyNamespace& ns, cstr searchTerm)
	{
		fuzzyMatches.clear();
		AppendFuzzyStringMatchSubspacesAndPublicFunctions(searchTerm, ns);
		std::stable_sort(fuzzyMatches.begin(), fuzzyMatches.end(),
			[](const FuzzyMatch& a, const FuzzyMatch& b) -> bool
			{
				return a.levenshteinDistance < b.levenshteinDistance;
			}
		);

		enum { MAX_SEARCH_RESULTS = 30 };
		for (int i = 0; i < MAX_SEARCH_RESULTS; i++)
		{
			if (i >= (int32)fuzzyMatches.size())
			{
				break;
			}

			char matchDisplayText[256];
			StackStringBuilder ssb(matchDisplayText, sizeof matchDisplayText);

			if (fuzzyMatches[i].ns != nullptr)
			{
				AppendFullName(*fuzzyMatches[i].ns, ssb);

				if (fuzzyMatches[i].publicFunction != nullptr)
				{
					ssb << "." << fuzzyMatches[i].publicFunction->PublicName();
					searchArrayResults.push_back({ std::string(matchDisplayText),&ns,fuzzyMatches[i].publicFunction });
				}
				else
				{
					searchArrayResults.push_back({ std::string(matchDisplayText),&ns,nullptr });
				}
			}
			else
			{
				searchArrayResults.push_back({ fuzzyMatches[i].candidate, &ns, nullptr });
			}
		}
	}

	void AppendToSearchTermsRecursive(ISxyNamespace& ns, cstr searchTerm, cstr fullSearchItem)
	{
		auto* dot = FindDot(searchTerm);
		if (*dot == '.')
		{
			char subspaceName[256];
			strncpy_s(subspaceName, searchTerm,  dot - searchTerm);

			for (int i = 0; i < ns.SubspaceCount(); ++i)
			{
				auto& subspace = ns[i];
				if (Eq(subspace.Name(), subspaceName))
				{
					AppendToSearchTermsRecursive(subspace, dot + 1, fullSearchItem);
					return;
				}
			}
		}
		else
		{
			for (int i = 0; i < ns.SubspaceCount(); ++i)
			{
				auto& subspace = ns[i];
				if (StartsWith(subspace.Name(), searchTerm))
				{
					char fullNameSpaceName[256];
					GetFullNamespaceName(fullNameSpaceName, sizeof fullNameSpaceName, subspace);
					searchArrayResults.push_back({ fullNameSpaceName,&subspace, nullptr });

					if (*searchTerm)
					{
						for (int j = 0; j < subspace.SubspaceCount(); ++j)
						{
							auto& subspaceChild = subspace[j];
							GetFullNamespaceName(fullNameSpaceName, sizeof fullNameSpaceName, subspaceChild);
							searchArrayResults.push_back({ fullNameSpaceName,&subspaceChild, nullptr });
						}
					}
				}
			}

			for (int j = 0; j < ns.FunctionCount(); ++j)
			{
				auto& function = ns.GetFunction(j);
				auto* name = function.PublicName();
				if (StartsWith(name, searchTerm))
				{
					char fullName[256];
					SafeFormat(fullName, fullSearchItem, searchTerm - fullSearchItem);
					size_t delta = strlen(searchTerm);
					StringCat(fullName, name + delta, (int32) sizeof fullName);
					searchArrayResults.push_back({ fullName,&ns,&function });
				}
			}
		}
	}

	void OnSearchChange(cstr searchTerm)
	{
		isDirty = true;

		searchArrayResults.clear();

		searchResults->ClearItems();

		auto& root = databaseSet.GetDatabase().GetRootNamespace();
		AppendToSearchTermsRecursive(root, searchTerm, searchTerm);

		if (*searchTerm != 0 && searchArrayResults.empty())
		{
			AppendFuzzyItemsToSearchTerms(root, searchTerm);
		}

		RefreshResultList();
	}

	void OnMouseMovedInSearchBar(Vec2i)
	{
		if (GetFocus() == searchEditor->OSEditor())
		{
			RefreshResultList();
		}
	}

	void SetSearchTerm(cstr searchTerm)
	{
		SafeFormat(searchTerms, "%s", searchTerm);
		searchEditor->Bind(searchTerms, sizeof searchTerms);
		int endIndex = Edit_LineLength(searchEditor->OSEditor(), 0);
		Edit_SetSel(searchEditor->OSEditor(), endIndex, endIndex);
		RefreshResultList();
	}

	ISxyNamespace* FindSubspace(ISxyNamespace& branch, cstr name)
	{
		for (int i = 0; i < branch.SubspaceCount(); ++i)
		{
			auto& subspace = branch[i];
			if (Eq(subspace.Name(), name))
			{
				return &subspace;
			}
		}

		return nullptr;
	}

	void SearchSelectedRecursive(cstr searchTerm, ISxyNamespace& ns)
	{
		auto* dot = FindDot(searchTerm);
		if (*dot == '.')
		{
			char subspaceName[256];
			strncpy_s(subspaceName, searchTerm, dot - searchTerm);

			ISxyNamespace* subspace = FindSubspace(ns, subspaceName);
			if (subspace)
			{
				SearchSelectedRecursive(dot + 1, *subspace);
			}
			// We could not find a match, so this namespace is taken to be the one to be highlighted
		}
		else
		{
			ISxyNamespace* subspace = FindSubspace(ns, searchTerm);
			if (subspace)
			{
				auto i = mapNSToTreeItemId.find(subspace);
				if (i != mapNSToTreeItemId.end())
				{
					TreeView_EnsureVisible(classTree->TreeWindow(), (HTREEITEM)i->second);
					TreeView_SelectItem(classTree->TreeWindow(), (HTREEITEM)i->second);
					return;
				}
			}

			auto i = mapNSToTreeItemId.find(&ns);
			if (i != mapNSToTreeItemId.end())
			{
				auto id = (ID_TREE_ITEM)TreeView_GetChild(classTree->TreeWindow(), i->second);
				while (id != 0)
				{
					char buf[1024];
					classTree->GetText(buf, sizeof buf, id);
					if (StartsWith(buf, searchTerm))
					{
						TreeView_EnsureVisible(classTree->TreeWindow(), id);
						TreeView_SelectItem(classTree->TreeWindow(), id);
						return;
					}

					id = (ID_TREE_ITEM)TreeView_GetNextSibling(classTree->TreeWindow(), id);
				}
			}
		}
	}

	void HilightClassItem(const SearchItem& item)
	{
		classTree->Collapse();
		
		if (item.function != nullptr)
		{
			auto i = mapPublicFunctionToTreeItem.find(item.function);
			if (i != mapPublicFunctionToTreeItem.end())
			{
				auto id = i->second;
				classTree->ExpandAt(id);
			}
		}
	}

	void OnEvent(Event& ev) override
	{
		if (ev == evMetaUpdated)
		{
			auto& evDB = As<TEventArgs<ISexyDatabase*>>(ev);
			OnMetaChanged(*evDB.value);
		}
		else if (ev == evSearchChange)
		{
			auto& search = As<TEventArgs<cstr>>(ev);
			OnSearchChange(search.value);
		}
		else if (ev == evMoveAtSearch)
		{
			auto& mousePosition = As<TEventArgs<Vec2i>>(ev);
			OnMouseMovedInSearchBar(mousePosition);
		}
		else if (ev == evDoubleClickSearchList)
		{
			auto& selectedItem = As<TEventArgs<std::pair<cstr, int>>>(ev);
			SetSearchTerm(selectedItem.value.first);
			int index = selectedItem.value.second;
			if (index >= 0 && index < searchArrayResults.size())
			{
				if (searchArrayResults[index].text == selectedItem.value.first)
				{
					HilightClassItem(searchArrayResults[index]);
				}
			}
		}
		else if (ev == evSearchSelected)
		{
			auto& selectedItem = As<TEventArgs<cstr>>(ev);
			SearchSelectedRecursive(selectedItem, databaseSet.GetDatabase().GetRootNamespace());
			searchResults->SetVisible(false);
			SetFocus(classTree->TreeWindow());
		}
	}

	char searchTerms[256] = { 0 };
	IAsciiStringEditor* searchEditor = nullptr;
	IFloatingListWidget* searchResults = nullptr;
public:
	SexyExplorer(WidgetContext _wc, ISplitScreen& _screen, ISexyDatabaseSet& _databaseSet, ISexyStudioEventHandler& _eventHandler, DefaulConfig& _config) : 
		wc(_wc), config(_config), screen(_screen), databaseSet(_databaseSet), eventHandler(_eventHandler)
	{
		screen.SetBackgroundColour(RGBAb(128, 128, 192));

		ITabSplitter* tabs = CreateTabSplitter(*screen.Children());
		tabs->SetVisible(true);

		classTab = &tabs->AddTab();
		classTab->SetName("Class View");

		IVariableList* searchBar = CreateVariableList(classTab->Children(), databaseSet.GetDatabase().PingPathResolver());
		searchBar->SetVisible(true);

		Widgets::AnchorToParentTop(*searchBar, 0);
		Widgets::AnchorToParentLeft(*searchBar, 0);
		Widgets::AnchorToParentRight(*searchBar, 0);
		Widgets::ExpandBottomFromTop(*searchBar, 32);

		searchEditor = searchBar->AddAsciiEditor();
		searchEditor->SetName("Search Editor");
		searchEditor->Bind(searchTerms, sizeof searchTerms);
		searchEditor->SetVisible(true);
		searchEditor->SetCharacterUpdateEvent(evSearchChange);
		searchEditor->SetUpdateEvent(evSearchSelected);
		searchEditor->SetMouseMoveEvent(evMoveAtSearch);

		searchResults = CreateFloatingListWidget(_screen, wc);
		searchResults->SetDoubleClickEvent(evDoubleClickSearchList);

		GuiRect rect{ 300, 300, 450, 600 };
		Widgets::SetWidgetPosition(*searchResults, rect);

		TreeStyle style;
		style.hasButtons = true;
		style.hasCheckBoxes = false;
		style.hasLines = true;
		classTree = CreateTree(classTab->Children(), style, *this);
		classTree->SetImageList(15, IDB_BLANK, IDB_NAMESPACE, IDB_INTERFACE, IDB_METHOD, IDB_STRUCT, IDB_STRONG, IDB_FIELD, IDB_EXTENDS, IDB_ATTRIBUTE, IDB_FUNCTION, IDB_INPUT, IDB_OUTPUT, IDB_ENUM, IDB_ALIAS, IDB_FACTORY, IDB_ARCHETYPE);

		SendMessageA(classTree->TreeWindow(), WM_SETFONT, (WPARAM) (HFONT) _wc.fontSmallLabel, 0);

		Widgets::AnchorToParent(*classTree, 0, 32, 0, 0);

		classTree->SetVisible(true);

		wc.publisher.Subscribe(this, evMetaUpdated);
		wc.publisher.Subscribe(this, evSearchChange);
		wc.publisher.Subscribe(this, evSearchSelected);
		wc.publisher.Subscribe(this, evMoveAtSearch);
		wc.publisher.Subscribe(this, evDoubleClickSearchList);
	}

	~SexyExplorer()
	{
		wc.publisher.Unsubscribe(this);
	}

	void SelectClassTreeTab()
	{
		classTab->Activate();
	}
};

LOGFONTA MakeDefaultFont()
{
	LOGFONTA lineEditorLF = { 0 };
	lineEditorLF.lfHeight = -12;
	lineEditorLF.lfCharSet = ANSI_CHARSET;
	lineEditorLF.lfOutPrecision = OUT_DEFAULT_PRECIS;
	lineEditorLF.lfClipPrecision = CLIP_DEFAULT_PRECIS;
	lineEditorLF.lfQuality = CLEARTYPE_QUALITY;
	SafeFormat(lineEditorLF.lfFaceName, "Consolas");
	return lineEditorLF;
}

using namespace Rococo::AutoComplete;

struct Factory;

void ShowPreviewPopup(IWindow& hParent, const char* token, const char* path, int lineNumber, IPreviewEventHandler& eventHandler);

const fstring sexyStudioDefaults = "SexyStudio.Defaults"_fstring;
const fstring configDefaultFullName = "Config.Default.FullName"_fstring;

struct SexyStudioIDE: ISexyStudioInstance1, IObserver, ICalltip, ISexyStudioGUI, ISexyStudioCompletionGaffer
{
	Factory& host;
	AutoFree<IPublisherSupervisor> publisher;
	AutoFree<ISexyDatabaseSupervisor> blankDatabase;
	DefaulConfig& config;
	Font smallCaptionFont;
	WidgetContext context;
	AutoFree<ITheme> theme;
	AutoFree<IIDEFrameSupervisor> ide;
	ISplitScreen* splitScreen = nullptr;
	ISplitScreen* projectView = nullptr;
	ISplitScreen* sourceView = nullptr;

	PropertySheets* sheets = nullptr;	// The left hand controls
	SexyExplorer* explorer = nullptr;	// The right hand controls

	int64 autoComplete_Replacement_StartPosition = 0;
	char callTipArgs[1024] = { 0 };

	std::vector<char> src_buffer;
	char src_line[1024];

	ISexyStudioEventHandler& eventHandler;

	ISexyStudioGUI& Gui() override
	{
		return *this;
	}

	ISexyStudioCompletionGaffer& Gaffer()
	{
		return *this;
	}

	void SetCalltipForReplacement(cstr tip) override
	{
		SafeFormat(callTipArgs, "%s", tip);
	}

	void PopupPreview(IWindow& hParent, cstr token, const char* path, int lineNumber, IPreviewEventHandler& eventHandler) override
	{
		ShowPreviewPopup(hParent, token, path, lineNumber, eventHandler);
	}

	void ReplaceCurrentSelectionWithCallTip(Rococo::AutoComplete::ISexyEditor& editor) override
	{
		int64 caretPos = editor.GetCaretPos();

		if (*callTipArgs != 0 && *callTipArgs != '!' && autoComplete_Replacement_StartPosition > 0 && autoComplete_Replacement_StartPosition < caretPos)
		{
			editor.ReplaceText(caretPos, caretPos, callTipArgs);
			callTipArgs[0] = 0;
		}
	}

	struct SpaceSeparatedStringItems : IEnumerator<cstr>
	{
		StringBuilder& sb;

		int count = 0;

		void operator()(cstr item) override
		{
			if (count > 0)
			{
				sb << " ";
			}

			count++;

			sb << item;
		}

		SpaceSeparatedStringItems(StringBuilder& _sb) : sb(_sb) {}
	};

	template<class ACTION> bool EnumerateFieldsOfClass(cr_substring className, cr_substring doc, ACTION& action)
	{
		cr_substring def = Rococo::Sex::Inference::GetClassDefinition(className, doc);
		if (def)
		{
			struct ANON: IFieldEnumerator
			{
				ACTION& action;

				void OnMemberVariable(cstr name, cstr type) override
				{
					if (Eq(type, "implements"_fstring))
					{

					}
					else
					{
						action(name);
					}
				}

				ANON(ACTION& _action) : action(_action) {}
			} buildList(action);

			Rococo::Sex::Inference::ForEachFieldOfClassDef(className, def, buildList);
			return true;
		}	

		return false;
	};

	struct RouteTextToAutoComplete: ISexyFieldEnumerator
	{
		IAutoCompleteBuilder& builder;
		ISexyDatabase& database;
		cr_substring prefix;
		cr_substring document;

		bool atLeastOneItem = false;

		int memberDepth = 0;

		Substring hint { Substring::Null() };

		RouteTextToAutoComplete(IAutoCompleteBuilder& _builder, cr_substring _prefix, ISexyDatabase& _database, cr_substring _document):
			builder(_builder), prefix(_prefix), database(_database), document(_document)
		{

		}

		~RouteTextToAutoComplete()
		{
			if (atLeastOneItem)
			{
				builder.ShowAndClearItems();
			}
		}

		void OnField(cstr fieldName, cr_substring memberSearch) override
		{
			atLeastOneItem = true;

			if (memberSearch && prefix)
			{
				char prefixString[128];
				prefix.CopyWithTruncate(prefixString, sizeof prefixString);
				size_t startSubstituteAt = memberSearch.start - prefix.start;
				size_t endSubstituteAt = startSubstituteAt + strlen(fieldName);
				if (endSubstituteAt < sizeof prefixString - 1)
				{
					memcpy(prefixString + startSubstituteAt, fieldName, strlen(fieldName) + 1);
					builder.AddItem(prefixString);
				}
			}
			else if (strstr(fieldName, "/@*"))
			{
				builder.AddItem(fieldName + 3);
			}
			else if (prefix)
			{
				char prefixString[128];
				prefix.CopyWithTruncate(prefixString, sizeof prefixString);

				char item[256];
				SafeFormat(item, "%s%s", prefixString, fieldName);
				builder.AddItem(item);
			}
			else
			{
				builder.AddItem(fieldName);
			}
		}

		void OnHintFound(cr_substring hintText) override
		{
			hint = hintText;
			builder.AddHint(hintText);
		}

		enum { MAX_MEMBER_DEPTH = 64 };

		void OnFieldType(cr_substring fieldType, cr_substring searchRoot) override
		{
			memberDepth++;
			if (!database.EnumerateVariableAndFieldList(searchRoot, fieldType, *this))
			{
				if (memberDepth > MAX_MEMBER_DEPTH)
				{
					if (OS::IsDebugging())
					{
						OS::TripDebugger();
					}
					memberDepth--;
					return;
				}
				Rococo::Sex::Inference::EnumerateLocalFields(*this, searchRoot, fieldType, document);
			}
			memberDepth--;
		}
	};

	void ShowCallTipAtCaretPos(ISexyEditor& editor, cr_substring tip)
	{
		size_t len = tip.Length() + 1;
		char* buf = (char*)alloca(len);
		tip.CopyWithTruncate(buf, len);
		editor.ShowCallTipAtCaretPos(buf);
	}

	void ShowAutocompleteDataForMacro(ISexyEditor& editor, cr_substring candidate, int64 displacementFromCaret, cr_substring doc)
	{
		int64 caretPos = editor.GetCaretPos();
		cstr openMacro = doc.start + caretPos - displacementFromCaret - 1;
		if (openMacro <= doc.start || *openMacro != '(')
		{
			// macro invocations have the form (#<name> ...)
			return;
		}

		Substring token = Rococo::Sex::Inference::GetFirstTokenFromLeft(candidate);
		token.start++;
		if (token.start >= token.finish || !IsAlphaNumeric(*token.start))
		{
			// We need (#<a> with at least one character a before we start autocomplete. Generally Sexy enumeration macros will have some standard prefix
		}

		RouteTextToAutoComplete routeTextToAutoComplete(editor.AutoCompleteBuilder(), Substring::Null(), GetDatabase(), doc);
		GetDatabase().ForEachAutoCompleteMacroCandidate(token, routeTextToAutoComplete);
	}

	void ShowAutocompleteDataForVariable(ISexyEditor& editor, cr_substring candidate, int64 tokenDisplacementFromCaret)
	{
		static auto thisDot = "this."_fstring;

		int64 caretPos = editor.GetCaretPos();

		Substring doc = CachedDoc(editor);

		cstr docCaretPos = doc.start + caretPos;
		cstr start = docCaretPos - tokenDisplacementFromCaret;
		cstr end = start + Length(candidate);

		Substring candidateInDoc{ start, end };

		Substring variable = { candidateInDoc.start, candidateInDoc.finish };
	
		if (StartsWith(variable, thisDot))
		{
			variable.start += thisDot.length;
		}

		RouteTextToAutoComplete routeTextToAutoComplete(editor.AutoCompleteBuilder(), candidateInDoc, GetDatabase(), doc);

		Rococo::Sex::Inference::TypeInference inference;
		bool isThis;
		inference = Rococo::Sex::Inference::GetLocalTypeFromCurrentDocument(isThis, candidateInDoc, doc);
		if (inference.declarationType)
		{
			if (isThis)
			{
				auto addFieldToAutocomplete = [&editor](cstr fieldName)
				{
					char qualifiedFieldName[128];
					SafeFormat(qualifiedFieldName, "this.%s", fieldName);
					editor.AutoCompleteBuilder().AddItem(qualifiedFieldName);
				};

				if (EnumerateFieldsOfClass(inference.declarationType, doc, addFieldToAutocomplete))
				{
					editor.AutoCompleteBuilder().ShowAndClearItems();
				}
				else
				{
					Substring finalType = routeTextToAutoComplete.hint ? routeTextToAutoComplete.hint : inference.declarationType;
					ShowCallTipAtCaretPos(editor, finalType);
				}
			}
			else
			{
				if (inference.templateContainer)
				{
					GetDatabase().EnumerateTemplateMethods(variable, inference, routeTextToAutoComplete);
				}
				else if (GetDatabase().EnumerateVariableAndFieldList(variable, inference.declarationType, routeTextToAutoComplete))
				{

				}
				else
				{	
					Rococo::Sex::Inference::EnumerateLocalFields(routeTextToAutoComplete, candidateInDoc, inference.declarationType, doc);
					if (!routeTextToAutoComplete.atLeastOneItem)
					{
						auto finalType = routeTextToAutoComplete.hint ? routeTextToAutoComplete.hint : inference.declarationType;
						ShowCallTipAtCaretPos(editor, finalType);
					}
				}
			}
		}
	}

	void ShowAutocompleteDataForType(ISexyEditor& editor, cr_substring candidate, cr_substring doc)
	{
		Substring token = Rococo::Sex::Inference::GetFirstTokenFromLeft(candidate);

		RouteTextToAutoComplete routeTextToAutoComplete(editor.AutoCompleteBuilder(), Substring::Null(), GetDatabase(), doc);
		GetDatabase().ForEachAutoCompleteCandidate(token, routeTextToAutoComplete);

		callTipArgs[0] = 0;

		if (!routeTextToAutoComplete.atLeastOneItem)
		{
			GetHintForCandidate(token, callTipArgs);
			if (callTipArgs[0] != 0)
			{
				editor.ShowCallTipAtCaretPos(callTipArgs);
			}
		}
	}

	void ShowFunctionArgumentsForType(ISexyEditor& editor, cr_substring candidate)
	{
		Substring token = Rococo::Sex::Inference::GetFirstTokenFromLeft(candidate);

		callTipArgs[0] = 0;

		GetHintForCandidate(token, callTipArgs);
		if (callTipArgs[0] != 0)
		{
			editor.ShowCallTipAtCaretPos(callTipArgs);
		}
	}

	bool TryAddTokenOptionsToAutocomplete(ISexyEditor& editor, cr_substring candidate, int64 displacementFromCaret, cr_substring doc)
	{
		using namespace Rococo;

		if (!candidate)
		{
			return false;
		}
		else if (Rococo::Sex::Inference::IsSexyKeyword(candidate))
		{
			return false;
		}
		else if (*candidate.start == '#')
		{
			ShowAutocompleteDataForMacro(editor, candidate, displacementFromCaret, doc);
			return true;
		}
		else if (islower(*candidate.start))
		{
			ShowAutocompleteDataForVariable(editor, candidate, displacementFromCaret);
			return true;
		}
		else if (isupper(*candidate.start))
		{
			ShowAutocompleteDataForType(editor, candidate, doc);
			return true;
		}

		return false;
	}

	IWindow& GetIDEFrame()
	{
		return ide->Window();
	}

	ISexyDatabase& GetDatabase()
	{
		return (currentProjectIndex == -1)  ? *blankDatabase : *projects[currentProjectIndex]->database;
	}

	const ISexyDatabase& GetDatabase() const
	{
		return  (currentProjectIndex == -1) ? *blankDatabase : *projects[currentProjectIndex]->database;
	}

	SexyStudioIDE(IWindow& topLevelWindow, ISexyStudioEventHandler& evHandler, DefaulConfig& _config, Factory& _host) :
		host(_host),
		config(_config),
		publisher(Rococo::Events::CreatePublisher()),
		blankDatabase(CreateSexyDatabase(_config)),
		smallCaptionFont(MakeDefaultFont()),
		context{ *publisher, smallCaptionFont },
		theme{ UseNamedTheme("Classic", context.publisher) },
		eventHandler(evHandler)
	{
		try
		{
			Rococo::OS::LoadUserSEXML(nullptr, sexyStudioDefaults,
				[this](const Sex::SEXML::ISEXMLDirectiveList& topLevelDirectives)
				{
					size_t index = 0;
					auto* directive = FindDirective(topLevelDirectives, sexyStudioDefaults, REF index);
					if (directive)
					{
						auto& aDefaultConfigFile = (*directive)[configDefaultFullName];
						cstr defaultConfigFile = AsString(aDefaultConfigFile).c_str();

						U8FilePath container;
						Assign(container, defaultConfigFile);
						IO::ToSysPath(container.buf);

						if (EndsWith(defaultConfigFile, "\\sexystudio.config.sexml") && IO::IsFileExistant(defaultConfigFile))
						{
							IO::MakeContainerDirectory(container.buf);
							SetConfigDirectory(container.buf);
						}
					}
				}
			);
		}
		catch (...)
		{

		}

		ide = CreateMainIDEFrame(context, topLevelWindow, evHandler);
		Widgets::SetText(*ide, "Sexy Studio");

		Vec2i desktopSpan = Windows::GetDesktopSpan();
		Vec2i initWindowSpan = { 1024, 600 };
		if (desktopSpan.x > 2500) initWindowSpan.x = 2048;
		if (desktopSpan.y >= 1440) initWindowSpan.y = 1024;

		Widgets::SetSpan(*ide, initWindowSpan.x, initWindowSpan.y);

		splitScreen = CreateSplitScreen(ide->Children());
		Widgets::AnchorToParent(*splitScreen, 0, 0, 0, 0);

		splitScreen->SetBackgroundColour(RGBAb(192, 128, 128));
		splitScreen->SplitIntoColumns(400);

		projectView = splitScreen->GetFirstHalf();
		sourceView = splitScreen->GetSecondHalf();

		sheets = new PropertySheets(config, *projectView, *ide, *this);
		explorer = new SexyExplorer(context, *sourceView, *this, eventHandler, config);

		publisher->Subscribe(this, evIDEClose);
		publisher->Subscribe(this, evIDEMax);
		publisher->Subscribe(this, evIDEMin);

		ide->SetVisible(true);
		splitScreen->SetVisible(true);
		ide->LayoutChildren();

		sheets->CollapseTree();

		/* Uncomment to put some HWND debugging info to the console
		AutoFree<IDynamicStringBuilder> heapStringBuilder = CreateDynamicStringBuilder(1024);
		auto& sb = heapStringBuilder->Builder();
		Rococo::SexyStudio::AppendDescendantsAndRectsToString(*ide, sb);
		puts(*sb);
		*/

		sheets->SelectProjectTab();
		explorer->SelectClassTreeTab();

	//	TEventArgs<bool> nullArgs;
	//	publisher->Publish(nullArgs, evConfigPathChange);
	}

	~SexyStudioIDE();
	
	void ReplaceSelectedText(Rococo::AutoComplete::ISexyEditor& editor, cstr item)
	{
		int64 caretPos = editor.GetCaretPos();
		if (autoComplete_Replacement_StartPosition > 0 && autoComplete_Replacement_StartPosition < caretPos)
		{
			editor.ReplaceText(autoComplete_Replacement_StartPosition, caretPos, item);
			UpdateAutoComplete(editor, nullptr);
		}
	}

	void SetHintToFunctionArguments(Rococo::AutoComplete::ISexyEditor&, const ISXYFunction& f, bool appendCloseParenthesis = true)
	{
		StackStringBuilder sb(callTipArgs, sizeof callTipArgs);
		
		for (int i = 0; i < f.InputCount(); ++i)
		{
			sb.AppendChar('(');

			EQualifier inputQualifier = f.InputQualifier(i);

			cstr qualifier;

			switch (inputQualifier)
			{
			default:
				qualifier = "";
				break;
			case EQualifier::Constant:
				qualifier = "const ";
				break;
			case EQualifier::Output:
				qualifier = "output ";
				break;
			case EQualifier::Ref:
				qualifier = "ref ";
				break;
			}
			
			sb << qualifier;

			sb << f.InputType(i);
			sb.AppendChar(' ');
			sb << f.InputName(i);
			sb.AppendChar(')');
		}

		sb << " -> ";

		for (int j = 0; j < f.OutputCount(); ++j)
		{
			sb.AppendChar('(');
			sb << f.OutputType(j);
			sb.AppendChar(' ');
			sb << f.OutputName(j);
			sb.AppendChar(')');
		}

		if (appendCloseParenthesis)
		{
			sb.AppendChar(')');
		}
	}

	const ISXYInterface* FindInterface(cr_substring type, const ISxyNamespace** ppNamespace = nullptr)
	{
		size_t len = type.Length() + 1;
		auto* buf = (char*)alloca(len);
		type.CopyWithTruncate(buf, len);
		return GetDatabase().FindInterface(buf, ppNamespace);
	}

	int interfaceDepth = 0;

	class InterfaceDepthCounter
	{
		SexyStudioIDE& This;
	public:
		InterfaceDepthCounter(SexyStudioIDE& ide): This(ide)
		{
			ide.interfaceDepth++;
		}
		~InterfaceDepthCounter()
		{
			This.interfaceDepth--;
		}
	};

	enum { MAX_INTERFACE_DEPTH = 64 };

	void FormatFactoryHint(cstr prefix, cstr suffix, char* buffer, size_t capacity, const ISXYFactory& factory, const ISxyNamespace* optionalNS)
	{
		StackStringBuilder sb(buffer, capacity);
		sb << prefix;

		if (optionalNS)
		{
			char fullname[256];
			GetFullNamespaceName(fullname, sizeof fullname, *optionalNS);
			sb << fullname;
			sb.AppendChar('.');
		}

		sb << factory.PublicName();

		for (int i = 0; i < factory.InputCount(); i++)
		{
			sb.AppendChar('(');
			sb << factory.InputType(i);
			sb.AppendChar(' ');
			sb << factory.InputName(i);
			sb.AppendChar(')');
		}
		sb << suffix;
	}

	NOT_INLINE bool TryShowCallTipForFactories(cr_substring type, cr_substring variableName, ISexyEditor& editor, ICalltip& calltip)
	{
		UNUSED(variableName);

		const ISxyNamespace* pNamespace = nullptr;
		auto* pInterface = FindInterface(type, &pNamespace);
		if (!pInterface)
		{
			return false;
		}

		char interfaceType[256];
		if (!type.TryCopyWithoutTruncate(interfaceType, sizeof interfaceType))
		{
			return false;
		}

		int candidates = 0;
		for (int i = 0; i < pNamespace->FactoryCount(); i++)
		{
			auto& f = pNamespace->GetFactory(i);

			char buf[256];
			f.GetDefinedInterface(buf, sizeof buf);

			if (EndsWith(buf, interfaceType))
			{
				candidates++;
			}
		}

		if (candidates == 1)
		{
			for (int i = 0; i < pNamespace->FactoryCount(); i++)
			{
				auto& f = pNamespace->GetFactory(i);

				char buf[256];
				f.GetDefinedInterface(buf, sizeof buf);

				if (EndsWith(buf, interfaceType))
				{
					char suggestion[256];
					FormatFactoryHint("(", "))", suggestion, sizeof  suggestion, f, pNamespace);
					editor.ShowCallTipAtCaretPos(suggestion);
					calltip.SetCalltipForReplacement(suggestion);
					break;
				}
			}
		}
		else if (candidates > 1)
		{
			char nsFullName[256];
			GetFullNamespaceName(nsFullName, sizeof nsFullName, *pNamespace);

			for (int i = 0; i < pNamespace->FactoryCount(); i++)
			{
				auto& f = pNamespace->GetFactory(i);

				char buf[256];
				f.GetDefinedInterface(buf, sizeof buf);

				if (EndsWith(buf, interfaceType))
				{
					char suggestion[256];
					StackStringBuilder sb(suggestion, sizeof suggestion);
					sb.AppendChar('(');		
					sb << nsFullName;
					sb.AppendChar('.');

					editor.AutoCompleteBuilder().AddItem(suggestion);
				}
			}
		}

		return true;
	}

	bool TryShowCallTipForMethods(cr_substring type, cr_substring methodName, ISexyEditor& editor)
	{
		InterfaceDepthCounter counter(*this);
	
		if (interfaceDepth > MAX_INTERFACE_DEPTH)
		{
			if (OS::IsDebugging())
			{
				OS::TripDebugger();
			}
			return false;
		}
	
		auto* pInterface = FindInterface(type);
		if (pInterface)
		{
			for (int i = 0; i < pInterface->MethodCount(); ++i)
			{
				auto& method = pInterface->GetMethod(i);
				auto sMethod = to_fstring(method.PublicName());
				if (Eq(sMethod, methodName))
				{
					SetHintToFunctionArguments(editor, method);
					editor.ShowCallTipAtCaretPos(callTipArgs);
					return true;
				}
			}

			auto* base = pInterface->Base();
			if (base)
			{
				bool result = TryShowCallTipForMethods(Substring::ToSubstring(base), methodName, editor);
				return result;
			}
		}

		return false;
	}

	bool TryGetFieldTypeOfType(char* fieldType, cstr type, cr_substring fieldName, cr_substring doc)
	{
		UNUSED(fieldType);
		UNUSED(type);
		UNUSED(fieldName);
		UNUSED(doc);
		if (OS::IsDebugging()) OS::TripDebugger();
		return false; // Not implemented
	}

	bool ValidateSubstringIsContainedWithin(cr_substring substring, cr_substring container)
	{
		if (substring.start < container.start || substring.start > container.finish)
		{
			if (OS::IsDebugging()) OS::TripDebugger();
			return false;
		}

		if (substring.finish < container.start || substring.finish > container.finish)
		{
			if (OS::IsDebugging()) OS::TripDebugger();
			return false;
		}
	}

	// Retrieves the searchTerm as it appears in the document
	Substring GetSearchTermInDoc(ISexyEditor& editor, cr_substring searchTerm, cr_substring doc)
	{
		cstr docCaretPos = doc.start + editor.GetCaretPos();
		int64 displacementFromCaret = searchTerm.Length();
		cstr start = docCaretPos - displacementFromCaret;
		cstr end = start + searchTerm.Length();
		return { start, end };
	}

	Substring GetTypeForMember(cr_substring type, cr_substring member, cr_substring doc)
	{
		struct: ISexyFieldEnumerator
		{
			Substring member{ Substring::Null() };
			Substring fieldType { Substring::Null() };

			void OnFieldType(cr_substring fieldType, cr_substring searchRoot) override
			{
				if (Eq(searchRoot, member))
				{
					this->fieldType = fieldType;
				}
			}

			void OnField(cstr fieldName, cr_substring memberSearch) override
			{
				UNUSED(fieldName);
				UNUSED(memberSearch);
			}

			void OnHintFound(cr_substring hint) override
			{
				UNUSED(hint);
			}

		} searchCallback;
		searchCallback.member = member;

		Rococo::Sex::Inference::EnumerateLocalFields(searchCallback, member, type, doc);

		return searchCallback.fieldType;
	}

	Substring GetSubType(cr_substring type, cstr subMember, cr_substring candidateInDoc, cr_substring doc)
	{		
		UNUSED(subMember);
		cstr nextDot = ForwardFind('.', candidateInDoc);
		if (!nextDot)
		{
			return Substring::Null();
		}

		Substring member = { candidateInDoc.start, nextDot };

		return GetTypeForMember(type, member, doc);
	}
	
	static bool IsWhitespace(char c)
	{
		switch (c)
		{
		case '\t':
		case ' ':
		case '\r':
		case '\n':
			return true;
		default:
			return false;
		}
	}

	bool TryFindAndShowCallTipForFactories(ISexyEditor& editor, cr_substring searchToken, cr_substring doc, ICalltip& calltip)
	{
		UNUSED(doc);

		if (!IsWhitespace(searchToken.finish[-1]))
		{
			return false;
		}

		// We are searching for the pattern '(<interface> <variable-name> '
		cstr lastOpenBrace = Rococo::Strings::ReverseFind('(', searchToken);
		if (lastOpenBrace == nullptr)
		{
			return false;
		}

		cstr lastCloseBrace = Rococo::Strings::ReverseFind(')', searchToken);
		if (lastCloseBrace != nullptr && lastCloseBrace > lastOpenBrace)
		{
			return false;
		}

		// We now have found '( .... '

		cstr interfaceName = nullptr;

		for (cstr p = lastOpenBrace + 1; p < searchToken.finish; p++)
		{
			if (IsWhitespace(*p))
			{
				continue;
			}

			if (!IsCapital(*p))
			{
				// Unknown character, so this cannot be a plain interface name
				return false;
			}

			interfaceName = p;
			break;
		}

		if (!interfaceName)
		{
			return false;
		}

		Substring interfaceNameAndArg = { interfaceName, searchToken.finish };

		cstr interfaceNameEnd = nullptr;
		for (cstr p = interfaceNameAndArg.start + 1; p < searchToken.finish; p++)
		{
			if (IsWhitespace(*p))
			{
				interfaceNameEnd = p;
				break;
			}

			if (IsAlphaNumeric(*p))
			{
				// Still in the name
				continue;
			}

			if (*p == '.')
			{
				continue;
			}

			// Something unexpected
			return false;
		}

		if (!interfaceNameEnd)
		{
			return false;
		}

		cstr variableArg = SkipBlankspace({ interfaceNameEnd, interfaceNameAndArg.finish });
		if (!variableArg)
		{
			return false;
		}

		// variables begin with lower case letters a-z
		if (!islower(*variableArg))
		{
			return false;
		}

		cstr variableArgFinish = nullptr;
		for (cstr p = variableArg; p < interfaceNameAndArg.finish; p++)
		{
			if (IsWhitespace(*p))
			{
				variableArgFinish = p;
				break;
			}

			if (IsAlphaNumeric(*p))
			{
				// Still in the name
				continue;
			}

			// Something unexpected
			return false;
		}

		if (!variableArgFinish)
		{
			return false;
		}

		Substring interfaceToken { interfaceNameAndArg.start, interfaceNameEnd };
		Substring variableToken { variableArg, variableArgFinish }; 

		return TryShowCallTipForFactories(interfaceToken, variableToken, editor, calltip);
	}

	bool TryFindAndShowCallTipForMethods(ISexyEditor& editor, cr_substring searchToken, cr_substring doc)
	{
		// Potentially a method call
		cstr methodSeparator = Rococo::Strings::ReverseFind('.', searchToken);
		if (!methodSeparator || *methodSeparator != '.')
		{
			return false;
		}

		if (methodSeparator >= searchToken.finish || !isupper(methodSeparator[1]))
		{
			return false;
		}

		// Potential method name, with left of separator being the interface variable

		Substring candidateInDoc = GetSearchTermInDoc(editor, searchToken, doc);

		Substring methodName{ methodSeparator + 1, searchToken.finish - 1 };

		Rococo::Sex::Inference::TypeInference type;
		bool isThis;
		type = Rococo::Sex::Inference::GetLocalTypeFromCurrentDocument(isThis, candidateInDoc, doc);

		if (type.declarationType)
		{
			Substring branchType = type.declarationType;
			Substring subsearch = searchToken;

			for (;;)
			{
				cstr firstDot = ForwardFind('.', subsearch);
				if (firstDot == methodSeparator)
				{
					return TryShowCallTipForMethods(branchType, methodName, editor);
				}
				else
				{
					subsearch = { firstDot + 1, subsearch.finish };
					Substring subType = GetSubType(branchType, firstDot + 1, subsearch, doc);
					if (!subType)
					{
						return false;
					}

					branchType = subType;
				}
			}
		}

		return false;
	}

	Substring CachedDoc(ISexyEditor& editor)
	{
		int64 nCharsAndNull = editor.GetDocLength();
		src_buffer.resize(nCharsAndNull + 1);
		editor.GetText(nCharsAndNull, src_buffer.data());
		src_buffer[nCharsAndNull] = 0;

		Substring doc;
		doc.start = src_buffer.data();
		doc.finish = doc.start + nCharsAndNull;

		return doc;
	}

	Substring GetCurrentLine(ISexyEditor& editor)
	{
		EditorLine currentLine(src_line, sizeof src_line);
		if (!editor.TryGetCurrentLine(currentLine))
		{
			return Substring{nullptr,nullptr};
		}

		return Substring { src_line, src_line + strlen(src_line) };
	}

	Substring GetSearchTokenWithinLine(cr_substring editorLine, const EditorCursor& cursor, cstr& activationPoint)
	{
		int64 caretColumn = cursor.CaretColumnNumber();
		if (caretColumn == 0)
		{
			return Substring::Null();
		}

		activationPoint = editorLine.start + caretColumn - 1;

		cstr openingToken = Rococo::Sex::Inference::GetFirstNonTokenPointerFromRight(editorLine, activationPoint);
		if (openingToken == nullptr)
		{
			openingToken = editorLine.start;
		}
		else
		{
			openingToken++; // this takes us into the alphanumeric string
		}

		Substring searchToken = Rococo::Sex::Inference::GetFirstTokenFromLeft({ openingToken, activationPoint + 1 });
		if (!searchToken)
		{
			return Substring::Null();
		}
		return { searchToken.start, activationPoint + 1 };
	}

	int64 LinePointerToDocPosition(const EditorCursor& cursor, cr_substring line, cstr linePointer)
	{
		return linePointer - line.start + cursor.lineStartPosition;
	}

	U8FilePath lastAutoRebasePath;

	struct Project: IFactoryConfig
	{
		AutoFree<ISexyDatabaseSupervisor> database;
		// std::wstring contentRoot;
		HString content;
		HString projectPath;
		std::wstring configPath;
		std::vector<SearchPathDesc> searchPaths;
		std::vector<HString> packages;

		Project()
		{

		}

		virtual ~Project()
		{

		}

		crwstr ConfigPath() const
		{
			return configPath.c_str();
		}

		Project(const char* sexyStudioConfigSexml)
		{
			WideFilePath wPath;
			Format(wPath, L"%hs", sexyStudioConfigSexml);

			configPath = wPath.buf;

			Rococo::OS::LoadSXMLBySysPath(sexyStudioConfigSexml,
				[this, sexyStudioConfigSexml](const Rococo::Sex::SEXML::ISEXMLDirectiveList& topLevelDirectives)
				{
					size_t startIndex = 0;
					auto& dDirectories = GetDirective(topLevelDirectives, "Directories", IN OUT startIndex);

					content = AsString(dDirectories["content"]).c_str();

					const fstring here = "$HERE$"_fstring;

					// We expect in many cases the config sexml will be in a descendant of the content directory
					// The container directory for the config sexml is given the macro $HERE$,
					// thus if content is say $ROCOCO$content and the config is $ROCOCO$content/config/sexystudio.config.sexml, then $HERE$ will map to $ROCOCO$content/config/,
					// so the content directory will be representable with $HERE$../

					if (StartsWith(content, here))
					{
						U8FilePath wHere;
						Assign(wHere, sexyStudioConfigSexml);
						IO::MakeContainerDirectory(wHere.buf);

						U8FilePath expandedContent;
						Format(expandedContent, "%s%s", wHere.buf, content.c_str() + here.length);

						IO::NormalizePath(expandedContent);

						content = expandedContent;
					}

					projectPath = AsString(dDirectories["project"]).c_str();;

					searchPaths.clear();

					size_t searchIndex = 0;
					while (auto* pSearchPath = FindDirective(dDirectories.Children(), "SearchPath", IN OUT searchIndex))
					{
						auto& dSearchPath = *pSearchPath;
						cstr path = AsString(dSearchPath["path"]).c_str();
						bool isActive = AsBool(dSearchPath["isActive"]);
						searchPaths.push_back({ path, isActive });
					}

					auto& dPackages = GetDirective(topLevelDirectives, "Packages", IN OUT startIndex);
					auto& aPackages = AsStringList(dPackages["selected-packages"]);
					for (int i = 0; i < aPackages.NumberOfElements(); i++)
					{
						fstring path = aPackages[i];
						packages.push_back((cstr)path);
					}
				}
			);

			if (!IO::IsDirectory(content))
			{
				Throw(0, "Expecting Directories/content in %s to be an existant directory.\nDirectory %s does not exist", sexyStudioConfigSexml, content.c_str());
			}
		}

		void Free()
		{
			delete this;
		}

		cstr GetProjectPath() const override
		{
			return projectPath;
		}

		SearchPathDescAtom GetSearchPath(size_t index) const override
		{
			if (index >= searchPaths.size())
			{
				return { nullptr, false };
			}

			auto& i = searchPaths[index];
			return { i.pingPath.c_str(), i.isActive };
		}

		void SetSearchPathActivity(size_t index, bool isActive)
		{
			if (index < searchPaths.size())
			{
				searchPaths[index].isActive = isActive;
			}
		}

		cstr GetPackage(size_t index) const override
		{
			if (index >= packages.size())
			{
				return nullptr;
			}

			return packages[index].c_str();
		}

		void SaveProtected()
		{
			// Se the Project constructor for an explanation of macro $HERE$
			try
			{
				crwstr wConfigPath = configPath.c_str();
				Rococo::OS::SaveSXMLBySysPath(wConfigPath, true,
					[this, wConfigPath](Rococo::Sex::SEXML::ISEXMLBuilder& sb)
					{
						sb.AddDirective("Directories");

						U8FilePath configPath;
						Format(configPath, "%ls", wConfigPath);

						if (StartsWith(configPath.buf, content))
						{
							U8FilePath macroedContentPath;
							StackStringBuilder macroBuilder(macroedContentPath.buf, U8FilePath::CAPACITY);
							macroBuilder << "$HERE$";

							U8FilePath reducedConfigPath = configPath;
							IO::MakeContainerDirectory(reducedConfigPath.buf);

							for (;;)
							{
								if (!IO::MakeContainerDirectory(reducedConfigPath.buf))
								{
									break;
								}

								if (!StartsWith(reducedConfigPath.buf, content))
								{
									break;
								}

								macroBuilder << "../";
							}

							sb.AddStringLiteral("content", macroedContentPath.buf);
						}
						else
						{
							sb.AddStringLiteral("content", content);
						}

						sb.AddStringLiteral("project", projectPath);
						for (auto& path : searchPaths)
						{
							sb.AddDirective("SearchPath");
							sb.AddStringLiteral("path", path.pingPath);
							sb.AddAtomicAttribute("isActive", path.isActive);
							sb.CloseDirective();
						}
						sb.CloseDirective(); // directories

						sb.AddDirective("Packages");
						sb.OpenListAttribute("selected-packages");

						for (auto& pck : packages)
						{
							sb.AddEscapedStringToList(pck);
						}

						sb.CloseListAttribute(); // known-packages
						sb.CloseDirective();
					}
				);
			}
			catch (IException& ex)
			{
				try
				{
					U8FilePath sexmlConfig;
					Rococo::OS::GetUserSEXMLFullPath(sexmlConfig, nullptr, "sexystudio.config");
					Throw(ex.ErrorCode(), "Error saving %s.\n%s", sexmlConfig.buf, ex.Message());
				}
				catch (IException& inner)
				{
					Windows::ShowErrorBox(Windows::NoParent(), inner, "SexyStudio: Save Error");
				}
			}
		}

		void Save()
		{
			try
			{
				SaveProtected();
			}
			catch (...)
			{

			}
		}
	};

	int FindProjectIndexByRoot(crwstr filename) const
	{
		for (int index = 0; index < projects.size(); index++)
		{
			WideFilePath wContent;
			Format(wContent, L"%hs", projects[index]->content.c_str());
			if (StartsWith(filename, wContent))
			{
				return index;
			}
		}

		return -1;
	}

	int FindProjectIndexByConfig(crwstr configFilename) const
	{
		for (int index = 0; index < projects.size(); index++)
		{
			if (EqI(configFilename, projects[index]->configPath.c_str()))
			{
				return index;
			}
		}

		return -1;
	}

	std::vector<AutoFree<Project>> projects;

	int AddNewProjectByConfig(const char* sexyStudioConfigSexml)
	{
		projects.emplace(projects.end(), new Project(sexyStudioConfigSexml));
		auto& p = *projects.back();
		p.database = CreateSexyDatabase(p);
		p.database->SetContentPath(p.content);
		return (int)(projects.size() - 1);
	}

	void SetConfigDirectory(cstr sysPathToConfigDirectory) override
	{
		U8FilePath sexyStudioConfig;
		Format(sexyStudioConfig, "%ssexystudio.config.sexml", sysPathToConfigDirectory);

		if (!IO::IsFileExistant(sexyStudioConfig))
		{
			Throw(0, "Could not find 'sexystudio.config.sexml' inside directory '%s'", sysPathToConfigDirectory);
		}

		WideFilePath wPath;
		Format(wPath, L"%hs", sexyStudioConfig.buf);

		crwstr configPath = wPath.buf;

		auto i = mapFilenameToProjectIndex.find(configPath);
		if (i == mapFilenameToProjectIndex.end())
		{
			int projectIndex = FindProjectIndexByConfig(configPath);
			if (projectIndex >= 0)
			{
				i = mapFilenameToProjectIndex.insert(std::pair<const std::wstring, int>(configPath, projectIndex)).first;
			}
			else
			{
				projectIndex = AddNewProject(configPath);
				i = mapFilenameToProjectIndex.insert(std::pair<const std::wstring, int>(configPath, projectIndex)).first;
			}
		}

		int projectIndex = i->second;

		if (projectIndex == -1)
		{
			return;
		}

		if (projectIndex != currentProjectIndex)
		{
			currentProjectIndex = projectIndex;
			if (sheets) sheets->SetContent(projects[currentProjectIndex]->content.c_str());
		}
	}

	// Returns the project index
	[[nodiscard]] int AddNewProject(crwstr fullPathToSXYfile)
	{
		U8FilePath u8Path;
		Assign(u8Path, fullPathToSXYfile);

		Substring s = Substring::ToSubstring(u8Path);

		// Recurse through container directories looking for sexystudio.config.sexml
		for (;;)
		{
			cstr lastSlash = Strings::ReverseFind(Rococo::IO::DirectorySeparatorChar(), s);
			if (!lastSlash)
			{
				break;
			}

			s = { s.start, lastSlash };

			*const_cast<char*>(lastSlash) = 0;

			U8FilePath sexyStudioConfig;
			Format(sexyStudioConfig, "%s%csexystudio.config.sexml", s.start, Rococo::IO::DirectorySeparatorChar());

			if (!IO::IsFileExistant(sexyStudioConfig))
			{
				continue;
			}

			try
			{
				return AddNewProjectByConfig(sexyStudioConfig);
			}
			catch (...)
			{
				break;
			}
		}

		// No project found
		return -1;
	}

	std::unordered_map<std::wstring, int> mapFilenameToProjectIndex;

	int currentProjectIndex = -1;

	void Rebase(crwstr fullPathToSXYfile)
	{
		if (!EndsWith(fullPathToSXYfile, L".sxy"))
		{
			return;
		}

		auto i = mapFilenameToProjectIndex.find(fullPathToSXYfile);
		if (i == mapFilenameToProjectIndex.end())
		{
			int projectIndex = FindProjectIndexByRoot(fullPathToSXYfile);
			if (projectIndex >= 0)
			{
				i = mapFilenameToProjectIndex.insert(std::pair<const std::wstring, int>(fullPathToSXYfile, projectIndex)).first;
			}
			else
			{
				projectIndex = AddNewProject(fullPathToSXYfile);
				i = mapFilenameToProjectIndex.insert(std::pair<const std::wstring, int>(fullPathToSXYfile, projectIndex)).first;
			}
		}

		int projectIndex = i->second;

		if (projectIndex == -1)
		{
			return;
		}

		if (projectIndex != currentProjectIndex)
		{
			currentProjectIndex = projectIndex;
			sheets->SetContent(projects[currentProjectIndex]->content.c_str());
		}
	}

	WideFilePath fullPathCache;

	static bool IsBlank(cr_substring sample)
	{
		for (cstr p = sample.start + 1; p < sample.finish; p++)
		{
			if (!isblank(*p))
			{
				return false;
			}
		}

		return true;
	}

	template<class LAMBDA>
	static void ForEachCompoundDirectiveWithAtLeastOneArg(const fstring fdirective, cr_substring sample, LAMBDA lambda)
	{
		Substring cursor = sample;

		while (true)
		{
			cstr directive = Strings::FindSubstring(cursor, fdirective);
			if (!directive)
			{
				break;
			}

			if (directive + fdirective.length >= sample.finish)
			{
				break;
			}

			Substring preUsing{ cursor.start, directive };
			cstr openParenthesis = Strings::ReverseFind('(', preUsing);

			cursor.start += fdirective.length + 2;

			if (openParenthesis)
			{
				Substring innerPadding{ openParenthesis + 1, directive };
				if (IsBlank(innerPadding))
				{
					// We have matched (<blankspace> <directive>...)

					if (isblank(directive[fdirective.length]))
					{
						// Our directive has blankspace after it: (<blankspace> <directive> <blankspace> ...)

						cstr nextArg = Strings::SkipBlankspace({ directive + fdirective.length, sample.finish });
						if (nextArg)
						{
							for (cstr p = nextArg + 1; p < sample.finish; p++)
							{
								if (isblank(*p) || *p == ')')
								{
									// We matched (<directive> <arg>)
									Substring arg{ nextArg, p };
									lambda(arg);

									cursor.start = p + 1;
									break;
								}
							}
						}
					}
				}
			}
		}
	}

	void PopulateImplicitNamespaces(cr_substring doc)
	{
		auto* implicits = GetDatabase().GetRootNamespace().ImplicitNamespaces();
		implicits->ClearImplicitNamespaces();

		ForEachCompoundDirectiveWithAtLeastOneArg("using"_fstring, doc, [this, &implicits](cr_substring fqNamespaceArg)
			{
				char nsBuffer[NAMESPACE_MAX_LENGTH];
				if (fqNamespaceArg.TryCopyWithoutTruncate(nsBuffer, sizeof nsBuffer))
				{
					bool unused = implicits->AddImplicitNamespace(nsBuffer);
					UNUSED(unused);
				}
			}
		);
	}

	std::vector<char> docTemp;

	int GetLineNumber(cstr doc, cstr target)
	{
		int lineNumber = 1;
		for (cstr p = doc; p < target; p++)
		{
			if (*p == '\n')
			{
				lineNumber++;
			}
		}
		return lineNumber;
	}

	void GotoDefinitionOfLowerCaseItem(cr_substring substringDoc, cstr doc, ISexyEditor& editor)
	{
		Sex::Inference::FaultTolerantSexyTypeInferenceEngine engine(doc);
		auto inference = engine.InferLocalVariableVariableType(substringDoc);
	
		if (inference.declarationType && inference.declarationVariable)
		{
			char type[256];
			inference.declarationType.CopyWithTruncate(type, sizeof type);

			char var[256];
			inference.declarationVariable.CopyWithTruncate(var, sizeof var);

			int lineNumber = GetLineNumber(doc, inference.declarationType.start);

			char docToken[256];
			substringDoc.CopyWithTruncate(docToken, sizeof docToken);

			editor.GotoDefinition(docToken, "<this>", lineNumber);
		}
	}

	void GotoDefinitionOfUpperCaseItem(cr_substring substringDoc, ISexyEditor& editor)
	{
		if (!substringDoc)
		{
			editor.GotoDefinition("?", "Huh?", 1);
			return;
		}

		char docToken[256];
		substringDoc.CopyWithTruncate(docToken, sizeof docToken);

		char fqName[256];
		if (!substringDoc.TryCopyWithoutTruncate(fqName, sizeof fqName))
		{
			return;
		}

		auto* type = GetDatabase().FindType(fqName);
		if (type)
		{
			auto* localType = type->LocalType();
			if (localType)
			{
				editor.GotoDefinition(docToken, localType->SourcePath(), localType->LineNumber());
			}

			return;
		}

		auto* pInterface = GetDatabase().FindInterface(fqName);
		if (pInterface)
		{
			editor.GotoDefinition(docToken, pInterface->SourcePath(), pInterface->GetDefinition().Start().y);
			return;
		}

		auto* f = GetDatabase().FindFunction(fqName);
		if (f)
		{
			auto* localFunction = f->LocalFunction();
			if (localFunction)
			{
				editor.GotoDefinition(docToken, localFunction->SourcePath(), localFunction->LineNumber());
				return;
			}
		}
	}

	NOT_INLINE void ExpandSearchTokenToRight(REF Substring& searchToken, cr_substring doc)
	{
		cstr p = searchToken.end();
		for (;p < doc.end(); p++)
		{
			if (IsAlphaNumeric(*p))
			{
				continue;
			}	
			else
			{
				// Blankspace or some other break such as / or .
				break;
			}
		}

		searchToken.finish = p;
	}

	Substring GetDocSubstringAtCaret(ISexyEditor& editor)
	{
		Substring substringLine = GetCurrentLine(editor);
		if (!substringLine)
		{
			return Substring::Null();
		}

		EditorCursor cursor;
		editor.GetCursor(cursor);

		Substring doc = CachedDoc(editor);

		cstr activationPoint;
		Substring searchToken = GetSearchTokenWithinLine(substringLine, cursor, activationPoint);

		if (!searchToken)
		{
			return Substring::Null();
		}

		int64 docPos = LinePointerToDocPosition(cursor, substringLine, searchToken.start);
		if (docPos < 0)
		{
			return Substring::Null();
		}

		size_t len = Length(searchToken);
		Substring s{ doc.start + docPos, doc.start + docPos + len };
		return s;
	}
	
	void GotoDefinitionOfSelectedToken(ISexyEditor& editor) override
	{		
		Substring substringDoc = GetDocSubstringAtCaret(editor);
		if (!substringDoc)
		{
			return;
		}
		ExpandSearchTokenToRight(REF substringDoc, CachedDoc(editor));
		
		if (!substringDoc)
		{
			editor.GotoDefinition("?", "Huh?", 1);
			return;
		}

		if (Eq("this", substringDoc))
		{
			// TODO - send us to the class
		}
		else if (islower(*substringDoc.start))
		{
			Substring doc = CachedDoc(editor);
			// token could be a keyword or variable identifier
			GotoDefinitionOfLowerCaseItem(substringDoc, doc.start, editor);
		}
		else if (isupper(*substringDoc.start))
		{
			// token could be a type, namespace or function name
			GotoDefinitionOfUpperCaseItem(substringDoc, editor);
		}
		else
		{
			editor.GotoDefinition("?", "Ayup?", 1);
		}
	}

	void UpdateAutoComplete(ISexyEditor& editor, crwstr fullPath) override
	{
		if (fullPath)
		{
			if (!Eq(fullPathCache, fullPath))
			{
				Format(fullPathCache, L"%s", fullPath);
				
				try
				{
					Rebase(fullPathCache);
					ide->SetVisible(false);
				}
				catch (IException& ex)
				{
					Windows::ShowErrorBox(Windows::NoParent(), ex, "SexyStudio threw an exception during autocomplete");
					return;
				}
			}
		}

		Substring substringLine = GetCurrentLine(editor);
		if (!substringLine)
		{
			return;
		}
	
		EditorCursor cursor;
		editor.GetCursor(cursor);

		Substring doc = CachedDoc(editor);

		cstr activationPoint;
		Substring searchToken = GetSearchTokenWithinLine(substringLine, cursor, activationPoint);

		if (!searchToken)
		{
			return;
		}

		char activationChar = *activationPoint;

		autoComplete_Replacement_StartPosition = LinePointerToDocPosition(cursor, substringLine, searchToken.start);

		// If blinking caret follows period or alphanumeric such as: Sys._ or Sys_, then we want to complete the dot.
		if (IsAlphaNumeric(activationChar) || activationChar == '.' || (isblank(activationChar) && *searchToken.start == '#'))
		{
			int64 displacementFromCaret = activationPoint - searchToken.start + 1;

			PopulateImplicitNamespaces(doc);

			if (!TryAddTokenOptionsToAutocomplete(editor, searchToken, displacementFromCaret, doc))
			{
				autoComplete_Replacement_StartPosition = 0;
			}
		}
		else if (activationChar == ' ' || activationChar == '\t')
		{
			// Potentially we have a method or function call followed by a space, which is a prompt to show the function arguments

			if (activationPoint > substringLine.start && IsAlphaNumeric(activationPoint[-1]))
			{
				if (isupper(*searchToken.start))
				{
					PopulateImplicitNamespaces(doc);

					// Potentially a function call
					ShowFunctionArgumentsForType(editor, searchToken);	
				}
				else if (islower(*searchToken.start))
				{
					PopulateImplicitNamespaces(doc);

					if (!TryFindAndShowCallTipForMethods(editor, searchToken, doc))
					{
						TryFindAndShowCallTipForFactories(editor, substringLine, doc, *this);
					}
				}
			}
		}
	}

	void SetTitle(cstr title) override
	{
		Widgets::SetText(ide->Window(), title);
	}

	void Activate() override
	{
		ShowWindow(ide->Window(), SW_RESTORE);
		SetForegroundWindow(ide->Window());
		BringWindowToTop(ide->Window());
		FlashWindow(ide->Window(), FALSE);
	}

	// Buffer should be 1024 bytes
	void GetHintForCandidate(cr_substring prefix, char args[1024]) override
	{
		GetDatabase().GetHintForCandidate(prefix, args);
	}

	bool isRunning = true;

	bool IsRunning() const override
	{
		return isRunning;
	}

	void Free() override
	{
		delete this;
	}

	void OnEvent(Rococo::Events::Event& ev) override
	{
		if (ev == evIDEClose)
		{
			isRunning = false;
		}
		else if (ev == evIDEMax)
		{
			Widgets::Maximize(ide->Window());
		}
		else if (ev == evIDEMin)
		{
			Widgets::Minimize(ide->Window());
		}
	}
};

HINSTANCE g_hDllInstance = nullptr;

BOOL WINAPI DllMain(HINSTANCE hDLL, DWORD fdwReason, LPVOID lpReserved)
{
	UNUSED(lpReserved);
	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
		g_hDllInstance = hDLL;
		break;

	case DLL_THREAD_ATTACH:
		// Do thread-specific initialization.
		break;

	case DLL_THREAD_DETACH:
		// Do thread-specific cleanup.
		break;

	case DLL_PROCESS_DETACH:
		// Perform any necessary cleanup.
		break;
	}
	return TRUE;
}

const char* const URL_base = "Rococo.SexyStudio.ISexyStudioBase";
const char* const URL_factory = "Rococo.SexyStudio.ISexyStudioFactory1";

struct Factory: Rococo::SexyStudio::ISexyStudioFactory1
{
	DefaulConfig config;
	int refCount = 1;

	Factory()
	{
		config.Load();
	}

	cstr GetInterfaceURL(int index) override
	{
		switch (index)
		{
		case 0:
			return URL_base;
		case 1:
			return URL_factory;
		default:
			return nullptr;
		}
	}

	cstr GetMetaDataString(EMetaDataType index) override
	{
		switch (index)
		{
		case EMetaDataType::BuildDate:
			return __DATE__ "@" __TIME__;
		case EMetaDataType::Copyright:
			return "Copyright(c) 2021. All rights reserved.";
		case EMetaDataType::Author:
			return "Mark Anthony Taylor";
		case EMetaDataType::Email:
			return "mark.anthony.taylor@gmail.com";
		default:
			return nullptr;
		}
	}

	ISexyStudioInstance1* CreateSexyIDE(IWindow& topLevelParent, ISexyStudioEventHandler& eventHandler) override
	{
		refCount++;

		try
		{
			ISexyStudioInstance1* ide = new SexyStudioIDE(topLevelParent, eventHandler, config, *this);
			ShowWindow(ide->Gui().GetIDEFrame(), SW_HIDE);
			return ide;
		}
		catch (...)
		{
			refCount--;
			throw;
		}
	}

	void Free() override
	{
		refCount--;
		if (refCount == 0)
		{
			delete this;
		}
	}
};

SexyStudioIDE::~SexyStudioIDE()
{
	try
	{
		crwstr configPath = GetDatabase().Config().ConfigPath();
		if (configPath)
		{
			U8FilePath u8Path;
			Assign(u8Path, configPath);

			Rococo::OS::SaveUserSEXML(nullptr, sexyStudioDefaults, true, 
				[this, &u8Path](Sex::SEXML::ISEXMLBuilder& builder)
				{
					builder.AddDirective(sexyStudioDefaults);
						builder.AddStringLiteral(configDefaultFullName, u8Path);
					builder.CloseDirective();
				}
			);
		}
	}
	catch (...)
	{

	}

	delete explorer;
	delete sheets;

	for (Project* p : projects)
	{
		p->Save();
	}

	config.Save();
	host.Free();
}

static bool isInitialized = false;

#include <rococo.sexml.h>
#include <rococo.functional.h>
#include <rococo.debugging.h>

extern "C" _declspec(dllexport) int CreateSexyStudioFactory(void** ppInterface, const char* interfaceURL)
{
	if (ppInterface == nullptr || interfaceURL == nullptr)
	{
		return E_POINTER;
	}

	if (!isInitialized)
	{
		HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
		if (FAILED(hr))
		{
			return CO_E_NOTINITIALIZED;
		}

		Rococo::OS::SetBreakPoints(Rococo::OS::Flags::BreakFlag_All & ~Rococo::OS::Flags::BreakFlag_IllFormed_SExpression);

		InitStudioWindows(g_hDllInstance, (LPCSTR)IDI_ICON1, (LPCSTR)IDI_ICON2);

		BufferedPaintInit();

		struct CLOSURE
		{
			static void OnExit()
			{
				BufferedPaintUnInit();
			}
		};

		atexit(CLOSURE::OnExit);

		isInitialized = true;
	}

	if (Eq(interfaceURL, URL_base) || Eq(interfaceURL, URL_factory))
	{
		*ppInterface = (void*) new Factory();
		return S_OK;
	}

	return E_NOTIMPL;
}