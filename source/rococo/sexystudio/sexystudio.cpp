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
#include <array>
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
#include "../sexystudio-lib/sexystudio.workspace.h"
#include "../sexystudio-lib/sexystudio.engine.h"
#include <rococo.hashtable.h>

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
	namespace
	{
		struct SourceTree final : ISourceTree
		{
			struct Item
			{
				HString path;
				int lineNumber;
			};
			std::unordered_map<SOURCE_TREE_ITEM_ID, Item> items;

			stringmap<int> mapSrcToCount;

			void Add(SOURCE_TREE_ITEM_ID item, cstr text, int lineNumber) override
			{
				U8FilePath nameAndLineNumber;
				Format(nameAndLineNumber, "%s#%d", text, lineNumber);
				auto i = mapSrcToCount.insert(nameAndLineNumber, 1);
				if (!i.second)
				{
					// Already existed
					i.first->second++;
				}
				else
				{
					items[item] = { text, lineNumber };
				}
			}

			void Clear() override
			{
				items.clear();
				mapSrcToCount.clear();
			}

			SourceAndLine Find(SOURCE_TREE_ITEM_ID item) const override
			{
				auto i = items.find(item);
				return i != items.end() ? SourceAndLine { i->second.path.c_str(), i->second.lineNumber } : SourceAndLine { nullptr, 0 };
			}

			void Free() override
			{
				delete this;
			}
		};
	} // anon

	ISourceTree* CreateSourceTree()
	{
		return new SourceTree();
	}
}

namespace Rococo::SexyStudio
{
	// Display only: lists the files under the database's active search paths in the tree, and maps the .sxy items to their paths in sourceTree.
	// It does not load, parse, clear or sort the database. The workspace populates the database, this shows it
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

		struct FileViewBuilder : IEventCallback<IO::FileItemData>
		{
			IGuiTree* tree;
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
						SafeFormat(progressText, "Listing SXY file...\r\n  %ls", item.fullPath);

						float percent = clamp(totalCount == 0 ? 50.0f : 100.0f * count / totalCount, 0.0f, 99.9f);
						progress->SetProgress(percent, progressText);

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
		} fileViewBuilder;

		fileViewBuilder.tree = &tree;
		fileViewBuilder.progress = &progress;
		fileViewBuilder.sourceTree = &sourceTree;
		fileViewBuilder.totalCount = fileCounter.count;

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
				Rococo::IO::ForEachFileInDirectory(wPath, fileViewBuilder.AppendFile(), true, (void*)hSearchPath);
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

		// The tree item ids are about to be reused, so the old id to path mapping must go, or right-click Open and Focus could pick a stale file
		sourceTree.Clear();

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

// The IDE's layout settings, kept in the user's sexystudio.layout. The project config, search paths, packages and their defaults belong to the workspace (sexystudio-lib)
struct DefaulConfig
{
	int searchPathHeight = 240;
	int packageViewFontHeight = -13;
	int searchViewFontHeight = -13;

	DefaulConfig()
	{
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

// The left hand controls: config directory, project, packages, search paths and the file browser. They display the workspace and pass the user's edits to it.
// They never populate or parse the database themselves: the workspace does that, and tells the sheets through IWorkspaceObserver
class PropertySheets: IObserver, IGuiTreeRenderer, IGuiTreeEvents, ISourceChangedEventHandler, IWorkspaceObserver
{
private:
	WidgetContext wc;
	DefaulConfig& config;
	IIDEFrame& ideFrame;
	ISexyWorkspace& workspace;
	AutoFree<ISourceTree> idToSourceMap = CreateSourceTree();
	IGuiTree* fileBrowser = nullptr;
	ITab* projectTab = nullptr;
	U8FilePath configDirectory;
	U8FilePath projectPath;
	IFilePathEditor* configPathEditor = nullptr;
	IFilePathEditor* projectDirEditor;
	bool isObservingWorkspace = false;
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
				Repopulate();
			}
			catch (...)
			{
				// Bad conversion
			}
		}
	}

	// The user changed a search path or package tick, or the project: ask the workspace to reload the current project. It calls OnWorkspaceChanged when done
	void Repopulate()
	{
		WaitCursorSection waitSection;
		ideFrame.SetProgress(0.0f, "Populating database...");

		try
		{
			workspace.Repopulate();
		}
		catch (IException& ex)
		{
			ShowErrorBox(ideFrame.Window(), ex, "Error populating the database");
		}

		ideFrame.SetProgress(100.0f, "Populated database");
	}

	// The user entered or browsed to a config directory: select that project, reloading it if it is already loaded, so changes on disk are picked up
	void OnConfigPathChanged()
	{
		if (!EndsWith(configDirectory, "\\"))
		{
			StringCat(configDirectory.buf, "\\", U8FilePath::CAPACITY);
		}

		WaitCursorSection waitSection;
		ideFrame.SetProgress(0.0f, "Loading project...");

		try
		{
			workspace.ReloadProjectByConfigDirectory(configDirectory);
		}
		catch (IException& ex)
		{
			ShowErrorBox(ideFrame.Window(), ex, "Error loading config: %s", configDirectory.buf);
		}

		ideFrame.SetProgress(100.0f, "Loaded project");
	}

	void OnWorkspaceChanged(ISexyDatabase& database, bool contentIsNew) override
	{
		if (contentIsNew && database.PopulateErrorCount() > 0)
		{
			ShowPopulateErrors(database);
		}

		Display(database);
	}

	void ShowPopulateErrors(ISexyDatabase& database)
	{
		char message[4096];
		StackStringBuilder sb(message, sizeof message);

		for (size_t i = 0; i < database.PopulateErrorCount(); i++)
		{
			cstr err = database.GetPopulateError(i);
			if (err)
			{
				sb << err << "\n";
			}
		}

		try
		{
			Throw(0, "%s", message);
		}
		catch (IException& ex)
		{
			ShowErrorBox(ideFrame.Window(), ex, "SexyStudio could not load all the packages");
		}
	}

	// Shows the workspace's current project: config directory, project, file browser, search paths and packages, then tells the class view. Reads only
	void Display(ISexyDatabase& database)
	{
		WaitCursorSection waitSection;

		crwstr wConfigPath = database.Config().ConfigPath();
		if (wConfigPath)
		{
			Assign(configDirectory, wConfigPath);
			IO::MakeContainerDirectory(configDirectory.buf);
			if (configPathEditor) configPathEditor->UpdateText();
		}

		Format(projectPath, "%s", database.Config().GetProjectPath());
		projectDirEditor->UpdateText();

		searchView->ClearItems();
		packageView->ClearItems();

		if (!workspace.HasProject())
		{
			// The blank database has no installation, hence no search paths or packages to show
			fileBrowser->Clear();
			idToSourceMap->Clear();
			return;
		}

		ideFrame.SetProgress(0.0f, "Populating file browser...");
		PopulateTreeWithSXYFiles(*fileBrowser, database, ideFrame, *idToSourceMap);
		ideFrame.SetProgress(100.0f, "Populated file browser");

		for (size_t index = 0;; index++)
		{
			auto atom = database.Config().GetSearchPath(index);
			if (atom.pingPath == nullptr)
			{
				break;
			}

			searchView->SetItem("Path", atom.pingPath, -1, atom.isActive ? 1 : 0);
		}

		try
		{
			U8FilePath sysPackagePath;
			database.PingPathResolver().PingPathToSysPath("!packages", sysPackagePath);

			PopulatePackageViewWithCheckboxes(sysPackagePath);
		}
		catch (...)
		{

		}

		TEventArgs<ISexyDatabase*> args;
		args.value = &database;
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
			workspace.GetDatabase().PingPathResolver().SysPathToPingPath(path, pingPath);

			bool isASelectedPackage = false;

			int j = 0;
			for (;;)
			{
				cstr p = workspace.GetDatabase().Config().GetPackage(j++);
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
			OnConfigPathChanged();
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
		// Write the ticks back to the project config, which the workspace then loads packages from
		auto& config = workspace.GetDatabase().Config();

		int nPackages = packageView->GetNumberOfRows();
		for (int i = 0; i < nPackages; i++)
		{
			U8FilePath pingPath;
			packageView->GetText(pingPath, i, 0);
			config.SetPackageSelected(pingPath, 1 == packageView->GetImageIndex(i, 0));
		}

		Repopulate();
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
				auto atom = workspace.GetDatabase().Config().GetSearchPath(index);
				if (atom.pingPath == nullptr)
				{
					break;
				}

				if (Eq(atom.pingPath, pingPath))
				{
					workspace.GetDatabase().Config().SetSearchPathActivity(index, isActive);
					break;
				}
			}
		}
		Repopulate();
	}
public:
	PropertySheets(DefaulConfig& _config, ISplitScreen& screen, IIDEFrame& _ideFrame, ISexyWorkspace& _workspace):
		wc(screen.Children()->Context()),
		config(_config),
		ideFrame(_ideFrame),
		workspace(_workspace),
		packageViewEventHandler(*this),
		searchPathEventHandler(*this)
	{
		Format(configDirectory, "%s", workspace.GetDatabase().Solution().GetContentFolder());
		Format(projectPath, "%s", workspace.GetDatabase().Config().GetProjectPath());

		screen.SetBackgroundColour(RGBAb(128, 192, 128));

		ITabSplitter* tabs = CreateTabSplitter(*screen.Children());
		tabs->SetVisible(true);

		projectTab = &tabs->AddTab();
		projectTab->SetName("Projects");
		projectTab->SetTooltip("Project View");


		IVariableList* projectSettings = CreateVariableList(projectTab->Children(), workspace.GetDatabase().PingPathResolver());
		projectSettings->SetVisible(true);

		Widgets::AnchorToParentTop(*projectSettings, 0);
		Widgets::AnchorToParentLeft(*projectSettings, 0);
		Widgets::AnchorToParentRight(*projectSettings, 0);

		configPathEditor = projectSettings->AddFilePathEditor(EFilePathType::SYS_PATHS);
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
		if (isObservingWorkspace)
		{
			workspace.RemoveObserver(*this);
		}

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

	// Starts displaying the workspace. The workspace replays its current state at once, so the sheets show the project that SexyStudio.Defaults selected,
	// and thereafter every project switch and repopulate. Call once the rest of the IDE (the class view in particular) is ready for evMetaUpdated
	void ObserveWorkspace()
	{
		if (!isObservingWorkspace)
		{
			isObservingWorkspace = true;
			workspace.AddObserver(*this);
		}
	}


	void CollapseTree()
	{
		fileBrowser->Collapse();
	}

	ISexyDatabase& Database() { return workspace.GetDatabase();  }

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
			OpenSexyFile(ideFrame.Events(), workspace.GetDatabase().Solution(), ideFrame.Window(), popupTargetFile.c_str(), popupTargetFileLine);
		}
		else if (id == COMMAND_FOCUS_PROJECT)
		{
			try
			{
				workspace.GetDatabase().FocusProject(popupTargetFile);

				workspace.GetDatabase().Sort();

				TEventArgs<ISexyDatabase*> args;
				args.value = &workspace.GetDatabase();
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
		if (ns.NumberOfMacroEnums() > 0)
		{
			auto idEnums = classTree->AppendItem(idNSNode);
			classTree->SetItemText(idEnums, "Enumerations");
			classTree->SetItemImage(idEnums, (int)ClassImageIndex::ENUM);

			for (int i = 0; i < ns.NumberOfMacroEnums(); ++i)
			{
				cstr enumName = ns.GetEnumMacroName(i);
				cstr enumText = ns.GetEnumMacroValue(i);

				auto idEnum = classTree->AppendItem(idEnums);

				char desc[256];
				SafeFormat(desc, "%s = %s", enumName, enumText);

				char fulldesc[256];
				SafeFormat(fulldesc, "%-64.64s %s", desc, ns.GetEnumMacroSourcePath(i));
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

// The user SEXML file in which the workspace records the last project, so the IDE opens it again next time
const fstring sexyStudioDefaults = "SexyStudio.Defaults"_fstring;

struct SexyStudioIDE: ISexyStudioInstance1, IObserver, ISexyStudioGUI, IEngineHost
{
	Factory& host;
	AutoFree<IPublisherSupervisor> publisher;
	AutoFree<ISexyWorkspace> workspace; // Projects, configs, databases and defaults. The IDE displays it and forwards ISexyDatabaseSet to it
	AutoFree<ISexyCompletionEngine> engine; // Completion on the workspace's current database. The IDE is its host and Gaffer() returns it
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

	ISexyStudioEventHandler& eventHandler;

	ISexyStudioGUI& Gui() override
	{
		return *this;
	}

	ISexyStudioCompletionGaffer& Gaffer()
	{
		return *engine;
	}

	// IEngineHost: the engine has rebased the workspace to the project of a new file. Hide the IDE, as completion always did
	void OnEditorFileChanged(crwstr fullPath) override
	{
		UNUSED(fullPath);
		ide->SetVisible(false);
	}

	// IEngineHost: completion failed and has been abandoned
	void OnEngineError(IException& ex) override
	{
		Windows::ShowErrorBox(Windows::NoParent(), ex, "SexyStudio threw an exception during autocomplete");
	}

	void PopupPreview(IWindow& hParent, cstr token, const char* path, int lineNumber, IPreviewEventHandler& eventHandler) override
	{
		ShowPreviewPopup(hParent, token, path, lineNumber, eventHandler);
	}

	IWindow& GetIDEFrame()
	{
		return ide->Window();
	}

	ISexyDatabase& GetDatabase()
	{
		return workspace->GetDatabase();
	}

	const ISexyDatabase& GetDatabase() const
	{
		return workspace->GetDatabase();
	}

	SexyStudioIDE(IWindow& topLevelWindow, ISexyStudioEventHandler& evHandler, DefaulConfig& _config, Factory& _host) :
		host(_host),
		config(_config),
		publisher(Rococo::Events::CreatePublisher()),
		workspace(CreateSexyWorkspace(sexyStudioDefaults.buffer)),
		engine(CreateSexyCompletionEngine(*workspace, *this)),
		smallCaptionFont(MakeDefaultFont()),
		context{ *publisher, smallCaptionFont },
		theme{ UseNamedTheme("Classic", context.publisher) },
		eventHandler(evHandler)
	{
		// Select the project the user last had open, if any. The property sheets display it once they observe the workspace, below
		workspace->LoadDefaults();

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

		sheets = new PropertySheets(config, *projectView, *ide, *workspace);
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

		// Now the class view exists, start displaying the workspace. Observing replays its current state at once, so a project that
		// SexyStudio.Defaults selected above is shown (file browser, search paths, packages, class view) without being parsed again
		sheets->ObserveWorkspace();

	//	TEventArgs<bool> nullArgs;
	//	publisher->Publish(nullArgs, evConfigPathChange);
	}

	~SexyStudioIDE();
	
	void SetDirectoryContainingConfigFile(cstr sysPathToConfigDirectory) override
	{
		workspace->SetDirectoryContainingConfigFile(sysPathToConfigDirectory);
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
	// Record the current project, so the next IDE opens it
	workspace->SaveDefaults();

	delete explorer;
	delete sheets; // Stops observing the workspace

	struct SaveErrorBox : IWorkspaceErrorHandler
	{
		void OnWorkspaceError(cstr context, IException& ex) override
		{
			Windows::ShowErrorBox(Windows::NoParent(), ex, "SexyStudio: Error saving %s", context);
		}
	} onSaveError;

	workspace->SaveProjects(&onSaveError);

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