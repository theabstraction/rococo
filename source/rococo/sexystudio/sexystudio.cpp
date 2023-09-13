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

#pragma comment(lib, "uxtheme.lib")
#pragma comment(lib, "ComCtl32.lib")

using namespace Rococo;
using namespace Rococo::SexyStudio;
using namespace Rococo::Events;
using namespace Rococo::Sex;
using namespace Rococo::Strings;

auto evClose = "EvMainIDEWindowCloseRequest"_event;
auto evContentChange = "EvContentChange"_event; // TEventArg<cstr>
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

void GetFullNamespaceName(char fullName[256], ISxyNamespace& ns)
{
	char temp[256] = { 0 };
	cstr writeEnd = temp + 254;
	char* writePos = (char*)writeEnd;

	for (ISxyNamespace* n = &ns; n->GetParent() != nullptr; n = n->GetParent())
	{
		size_t len = strlen(n->Name());
		writePos -= (len + 1);
		*writePos = '.';
		memcpy(writePos + 1, n->Name(), len);
	}

	CopyString(fullName, 256, writePos + 1);
}

using namespace Rococo::Sex::SEXML;

struct SearchPathDesc
{
	HString pingPath;
	bool isActive;
};

struct FactoryConfig: IFactoryConfig
{
	std::vector<SearchPathDesc> searchPaths;
	std::vector<HString> packages;
	HString content;
	HString projectPath;
	int searchPathHeight = 240;
	int packageViewFontHeight = -13;
	int searchViewFontHeight = -13;

	U8FilePath fileName;

	FactoryConfig()
	{
		Rococo::OS::GetUserSEXMLFullPath(fileName, nullptr, "sexystudio.config");
	}

	SearchPathDescAtom GetSearchPath(size_t index) const override
	{
		if (index == searchPaths.size())
		{
			return { projectPath.c_str(), true };
		}

		if (index > searchPaths.size())
		{
			return { nullptr, false };
		}

		auto& i = searchPaths[index];
		return { i.pingPath.c_str(), i.isActive };
	}

	void CreateLayoutFile()
	{
		Rococo::OS::SaveUserSEXML(nullptr, "sexystudio.layout",
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
			if (!Rococo::OS::IsUserSEXMLExistant(nullptr, "sexystudio.config"))
			{
				// Create it
				Rococo::OS::SaveUserSEXML(nullptr, "sexystudio.config",
					[](Rococo::Sex::SEXML::ISEXMLBuilder& sb)
					{
						sb.AddDirective("Directories");
							sb.AddStringLiteral("content", "C:\\work\\rococo\\content\\");
							sb.AddStringLiteral("project", "!scripts/mhost");
							
							sb.AddDirective("SearchPath");
								sb.AddStringLiteral("path", "!scripts/interop");
								sb.AddAtomicAttribute("isActive", true);
							sb.CloseDirective(); // search-paths

							sb.AddDirective("SearchPath");
								sb.AddStringLiteral("path", "!scripts/declarations");
								sb.AddAtomicAttribute("isActive", true);
							sb.CloseDirective(); // search-paths

							sb.AddDirective("SearchPath");
								sb.AddStringLiteral("path", "!scripts/native");
								sb.AddAtomicAttribute("isActive", true);
							sb.CloseDirective(); // search-paths

						sb.CloseDirective(); // directories

						sb.AddDirective("Packages");
							sb.OpenListAttribute("selected-packages");
								sb.AddEscapedStringToList("!packages/mhost_1000.sxyz");
							sb.CloseListAttribute(); // known-packages
						sb.CloseDirective();
					}
				);
			}

			if (!Rococo::OS::IsUserSEXMLExistant(nullptr, "sexystudio.layout"))
			{
				// Create it
				CreateLayoutFile();
			}

			Rococo::OS::LoadUserSEXML(nullptr, "sexystudio.config",
				[this](const Rococo::Sex::SEXML::ISEXMLDirectiveList& topLevelDirectives)
				{
					size_t startIndex = 0;
					auto& dDirectories = GetDirective(topLevelDirectives, "Directories", IN OUT startIndex);

					content = AsString(dDirectories["content"]).c_str();
					projectPath = AsString(dDirectories["project"]).c_str();;

					searchPaths.clear();

					size_t searchIndex = 0;
					while (auto* pSearchPath = FindDirective(dDirectories.Children(), "SearchPath", IN OUT searchIndex))
					{
						auto& dSearchPath = *pSearchPath;
						cstr path = AsString(dSearchPath["path"]).c_str();
						bool isActive = AsBool(dSearchPath["isActive"]);
						searchPaths.push_back({path, isActive});
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
		}
		catch (ParseException& ex)
		{
			try
			{
				U8FilePath filename;
				Rococo::OS::GetUserSEXMLFullPath(filename, nullptr, "sexystudio.config");
				Throw(ex.ErrorCode(), "Error parsing %s line %d pos %d. Err: %s\n\t", filename.buf, ex.Start().y, ex.Start().x, ex.Message());
			}
			catch(IException& inner)
			{
				Rococo::Debugging::AddCriticalLog(inner.Message());
			}
		}
		catch (IException& ex)
		{
			OS::TripDebugger();
			Rococo::Debugging::AddCriticalLog(ex.Message());
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
		try
		{
			Rococo::OS::SaveUserSEXML(nullptr, "sexystudio.config",
				[this](Rococo::Sex::SEXML::ISEXMLBuilder& sb)
				{
					sb.AddDirective("Directories");
					sb.AddStringLiteral("content", content);
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
	FactoryConfig& config;
	IIDEFrame& ideFrame;
	ISexyDatabase& database;
	AutoFree<ISourceTree> idToSourceMap = CreateSourceTree();
	IGuiTree* fileBrowser = nullptr;
	ITab* projectTab = nullptr;
	U8FilePath contentPath;
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
				config.projectPath = projectPath;
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

		if (!EndsWith(contentPath, "\\"))
		{
			StringCat(contentPath.buf, "\\", U8FilePath::CAPACITY);
		}

		database.Solution().SetContentFolder(contentPath);

		config.content = contentPath;

		PopulateTreeWithSXYFiles(*fileBrowser, database, ideFrame, *idToSourceMap);
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
					database.PingPathToSysPath(pingPath, sysPath);
					PopulateTreeWithPackage(sysPath, database);
				}
			}
		}

		database.Sort();

		TEventArgs<ISexyDatabase*> args;
		args.value = &database;
		wc.publisher.Publish(args, evMetaUpdated);
	}

	void PopulatePackageViewWithCheckboxes(cstr packagePath)
	{
		AutoFree<IO::IPathCacheSupervisor> pathCache = IO::CreatePathCache();

		try
		{
			pathCache->AddPathsFromDirectory(packagePath, false);
			pathCache->Sort();
		}
		catch (IException& ex)
		{
			Windows::ShowErrorBox(ideFrame, ex, "SexyStudio package reading issue");
		}

		packageView->ClearItems();

		for (size_t i = 0; i < pathCache->NumberOfFiles(); ++i)
		{
			cstr path = pathCache->GetFileName(i);

			U8FilePath pingPath;
			database.SysPathToPingPath(path, pingPath);

			bool isASelectedPackage = false;

			for (auto& selectedPackage : config.packages)
			{
				if (Eq(selectedPackage, pingPath))
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
		if (ev == evContentChange)
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
			
			for (auto& atom : config.searchPaths)
			{
				if (Eq(atom.pingPath, pingPath))
				{
					atom.isActive = isActive;
					break;
				}
			}
		}
		SyncContent();
	}
public:
	PropertySheets(ISplitScreen& screen, IIDEFrame& _ideFrame, ISexyDatabase& _database, FactoryConfig& _config): 
		wc(screen.Children()->Context()),
		config(_config),
		ideFrame(_ideFrame),
		database(_database),
		packageViewEventHandler(*this),
		searchPathEventHandler(*this)
	{
		Format(contentPath, "%s", database.Solution().GetContentFolder());
		Format(projectPath, "%s", config.projectPath.c_str());

		screen.SetBackgroundColour(RGBAb(128, 192, 128));

		ITabSplitter* tabs = CreateTabSplitter(*screen.Children());
		tabs->SetVisible(true);

		projectTab = &tabs->AddTab();
		projectTab->SetName("Projects");
		projectTab->SetTooltip("Project View");


		IVariableList* projectSettings = CreateVariableList(projectTab->Children(), database);
		projectSettings->SetVisible(true);

		Widgets::AnchorToParentTop(*projectSettings, 0);
		Widgets::AnchorToParentLeft(*projectSettings, 0);
		Widgets::AnchorToParentRight(*projectSettings, 0);

		auto* contentEditor = projectSettings->AddFilePathEditor(EFilePathType::SYS_PATHS);
		contentEditor->SetName("Content");

		contentEditor->Bind(contentPath, 128);
		contentEditor->SetVisible(true);
		contentEditor->SetUpdateEvent(evContentChange);

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

		for (auto& path : config.searchPaths)
		{
			searchView->SetItem("Path", path.pingPath, -1, path.isActive ? 1 : 0);
		}

		U8FilePath sysPackagePath;
		database.PingPathToSysPath("!packages", sysPackagePath);

		PopulatePackageViewWithCheckboxes(sysPackagePath);

		Widgets::ExpandBottomFromTop(*projectSettings, projectSettings->GetDefaultHeight());

		TreeStyle style;
		style.hasButtons = true;
		style.hasLines = true;

		fileBrowser = CreateTree(projectTab->Children(), style, *this, this);
		Widgets::AnchorToParent(*fileBrowser, 0, projectSettings->GetDefaultHeight(), 0, 0);

		fileBrowser->SetVisible(true);
		fileBrowser->SetImageList(4, IDB_FOLDER_CLOSED, IDB_FOLDER_OPEN, IDB_FILETYPE_SXY, IDB_FILETYPE_UNKNOWN);

		ideFrame.SetProgress(100.0f, "Complete!");

		wc.publisher.Subscribe(this, evContentChange);
		wc.publisher.Subscribe(this, evProjectChange);
	}

	~PropertySheets()
	{
		wc.publisher.Unsubscribe(this);

		config.packages.clear();

		for (int i = 0; i < packageView->GetNumberOfRows(); ++i)
		{
			int imageIndex = packageView->GetImageIndex(i, 0);
			if (imageIndex == 1)
			{
				U8FilePath path;
				packageView->GetText(path, i, 0);
				if (*path == '!')
				{
					config.packages.push_back((cstr)path);
				}
			}
		}
	}

	void SetContent(const U8FilePath& newPath)
	{
		if (Eq(contentPath, newPath))
		{
			return;
		}

		contentPath = newPath;

		SyncContent();
	}


	void CollapseTree()
	{
		fileBrowser->Collapse();
	}

	ISexyDatabase& Database() { return database;  }

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
			OpenSexyFile(ideFrame.Events(), database.Solution(), ideFrame.Window(), popupTargetFile.c_str(), popupTargetFileLine);
		}
		else if (id == COMMAND_FOCUS_PROJECT)
		{
			try
			{
				database.FocusProject(popupTargetFile);

				database.Sort();

				TEventArgs<ISexyDatabase*> args;
				args.value = &database;
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
	FactoryConfig& config;
	ISplitScreen& screen;
	ISexyDatabase& database;
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
			OpenSexyFile(eventHandler, database.Solution(), screen.Window(), popupSourceModule, popupSourceModuleLineNumber);
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
			SafeFormat(desc, "%s %s", archetype.InputType(k), archetype.InputName(k));
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

			if (localType->IsStrong())
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
		auto& root = database.GetRootNamespace();

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
					GetFullNamespaceName(fullNameSpaceName, subspace);
					searchArrayResults.push_back({ fullNameSpaceName,&subspace, nullptr });

					if (*searchTerm)
					{
						for (int j = 0; j < subspace.SubspaceCount(); ++j)
						{
							auto& subspaceChild = subspace[j];
							GetFullNamespaceName(fullNameSpaceName, subspaceChild);
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

		auto& root = database.GetRootNamespace();
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
			SearchSelectedRecursive(selectedItem, database.GetRootNamespace());
			searchResults->SetVisible(false);
			SetFocus(classTree->TreeWindow());
		}
	}

	char searchTerms[256] = { 0 };
	IAsciiStringEditor* searchEditor = nullptr;
	IFloatingListWidget* searchResults = nullptr;
public:
	SexyExplorer(WidgetContext _wc, ISplitScreen& _screen, ISexyDatabase& _database, ISexyStudioEventHandler& _eventHandler, FactoryConfig& _config) : 
		wc(_wc), config(_config), screen(_screen), database(_database), eventHandler(_eventHandler)
	{
		screen.SetBackgroundColour(RGBAb(128, 128, 192));

		ITabSplitter* tabs = CreateTabSplitter(*screen.Children());
		tabs->SetVisible(true);

		classTab = &tabs->AddTab();
		classTab->SetName("Class View");

		IVariableList* searchBar = CreateVariableList(classTab->Children(), database);
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

struct SexyStudioIDE: ISexyStudioInstance1, IObserver, ICalltip
{
	AutoFree<IPublisherSupervisor> publisher;
	AutoFree<ISexyDatabaseSupervisor> database;
	FactoryConfig& config;
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

	void SetCalltipForReplacement(cstr tip) override
	{
		SafeFormat(callTipArgs, "%s", tip);
	}

	void ReplaceCurrentSelectionWithCallTip(Rococo::AutoComplete::ISexyEditor& editor)
	{
		int64 caretPos = editor.GetCaretPos();

		if (*callTipArgs != 0 && autoComplete_Replacement_StartPosition > 0 && autoComplete_Replacement_StartPosition < caretPos)
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
				CopyWithTruncate(prefix, prefixString, sizeof prefixString);
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
				CopyWithTruncate(prefix, prefixString, sizeof prefixString);

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
		CopyWithTruncate(tip, buf, len);
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

		RouteTextToAutoComplete routeTextToAutoComplete(editor.AutoCompleteBuilder(), Substring_Null(), *database, doc);
		database->ForEachAutoCompleteMacroCandidate(token, routeTextToAutoComplete);
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

		RouteTextToAutoComplete routeTextToAutoComplete(editor.AutoCompleteBuilder(), candidateInDoc, *database, doc);

		Substring type;
		bool isThis;
		if (type = Rococo::Sex::Inference::GetLocalTypeFromCurrentDocument(isThis, candidateInDoc, doc))
		{
			if (isThis)
			{
				auto addFieldToAutocomplete = [&editor](cstr fieldName)
				{
					char qualifiedFieldName[128];
					SafeFormat(qualifiedFieldName, "this.%s", fieldName);
					editor.AutoCompleteBuilder().AddItem(qualifiedFieldName);
				};

				if (EnumerateFieldsOfClass(type, doc, addFieldToAutocomplete))
				{
					editor.AutoCompleteBuilder().ShowAndClearItems();
				}
				else
				{
					Substring finalType = routeTextToAutoComplete.hint ? routeTextToAutoComplete.hint : type;
					ShowCallTipAtCaretPos(editor, finalType);
				}
			}
			else
			{
				if (database->EnumerateVariableAndFieldList(variable, type, routeTextToAutoComplete))
				{

				}
				else
				{	
					Rococo::Sex::Inference::EnumerateLocalFields(routeTextToAutoComplete, candidateInDoc, type, doc);
					if (!routeTextToAutoComplete.atLeastOneItem)
					{
						auto finalType = routeTextToAutoComplete.hint ? routeTextToAutoComplete.hint : type;
						ShowCallTipAtCaretPos(editor, finalType);
					}
				}
			}
		}
	}

	void ShowAutocompleteDataForType(ISexyEditor& editor, cr_substring candidate, cr_substring doc)
	{
		Substring token = Rococo::Sex::Inference::GetFirstTokenFromLeft(candidate);

		RouteTextToAutoComplete routeTextToAutoComplete(editor.AutoCompleteBuilder(), Substring_Null(), *database, doc);
		database->ForEachAutoCompleteCandidate(token, routeTextToAutoComplete);

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
		return *database;
	}

	SexyStudioIDE(IWindow& topLevelWindow, ISexyStudioEventHandler& evHandler, FactoryConfig& _config) :
		config(_config),
		publisher(Rococo::Events::CreatePublisher()),
		database(CreateSexyDatabase(_config)),
		smallCaptionFont(MakeDefaultFont()),
		context{ *publisher, smallCaptionFont },
		theme{ UseNamedTheme("Classic", context.publisher) },
		eventHandler(evHandler)
	{
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

		int failcount = 0;
		for (;;)
		{
			try
			{
				try
				{
					database->SetContentPath(config.content);
					if (failcount > 0)
					{
						config.Save();
					}
					break;
				}
				catch (IException& ex)
				{
					failcount++;
					Throw(ex.ErrorCode(), "Error initializing SexyStudio database with content from %s @directories[\"content\"]\n%s", config.fileName, ex.Message());
				}
			}
			catch (IException& ex)
			{
				Rococo::Windows::ShowErrorBox(topLevelWindow, ex, "Sexy Studio Error");

				U8FilePath path;
				Assign(path, "C:\\");
				if (!IO::ChooseDirectory(path.buf, path.CAPACITY-1, "SexyStudio - please select the content folder"))
				{
					break;
				}

				if (IO::IsDirectory(path))
				{
					if (!EndsWith(path, "\\"))
					{
						StringCat(path.buf, "\\", path.CAPACITY);
					}
				}

				config.content = path;
			}
		}
	

		sheets = new PropertySheets(*projectView, *ide, *database, config);
		explorer = new SexyExplorer(context, *sourceView, *database, eventHandler, config);

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

		TEventArgs<bool> nullArgs;
		publisher->Publish(nullArgs, evContentChange);
	}

	~SexyStudioIDE()
	{
		delete explorer;
		delete sheets;

		config.Save();
	}

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

	ISXYInterface* FindInterface(cr_substring type, ISxyNamespace** ppNamespace = nullptr)
	{
		size_t len = type.Length() + 1;
		auto* buf = (char*)alloca(len);
		CopyWithTruncate(type, buf, len);
		return database->FindInterface(buf, ppNamespace);
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

	void FormatFactoryHint(cstr prefix, cstr suffix, char* buffer, size_t capacity, ISXYFactory& factory, ISxyNamespace* optionalNS)
	{
		StackStringBuilder sb(buffer, capacity);
		sb << prefix;

		if (optionalNS)
		{
			char fullname[256];
			GetFullNamespaceName(fullname, *optionalNS);
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

		ISxyNamespace* pNamespace = nullptr;
		auto* pInterface = FindInterface(type, &pNamespace);
		if (!pInterface)
		{
			return false;
		}

		char interfaceType[256];
		SubstringToString(interfaceType, sizeof interfaceType, type);

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
			GetFullNamespaceName(nsFullName, *pNamespace);

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
				bool result = TryShowCallTipForMethods(ToSubstring(base), methodName, editor);
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
		cstr lastOpenBrace = Rococo::ReverseFind('(', searchToken);
		if (lastOpenBrace == nullptr)
		{
			return false;
		}

		cstr lastCloseBrace = Rococo::ReverseFind(')', searchToken);
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
		cstr methodSeparator = Rococo::ReverseFind('.', searchToken);
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

		Substring type;
		bool isThis;
		if (type = Rococo::Sex::Inference::GetLocalTypeFromCurrentDocument(isThis, candidateInDoc, doc))
		{
			Substring branchType = type;
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

	void Rebase(const wchar_t* fullPathToSXYfile)
	{
		U8FilePath u8Path;
		Assign(u8Path, fullPathToSXYfile);

		if (!EndsWith(u8Path, ".sxy"))
		{
			return;
		}

		Substring s = ToSubstring(u8Path);

		for (;;)
		{
			cstr lastSlash = Strings::ReverseFind(Rococo::IO::DirectorySeparatorChar(), s);
			if (!lastSlash)
			{
				return;
			}

			s = { s.start, lastSlash };

			cstr earlierSlash = Strings::ReverseFind(Rococo::IO::DirectorySeparatorChar(), s);

			if (!earlierSlash)
			{
				return;
			}

			Substring candidate = { earlierSlash + 1, lastSlash };

			if (Eq(candidate, "content") || Eq(candidate, "Content") || Eq(candidate, "CONTENT"))
			{
				U8FilePath contentPath;
				CopyWithTruncate({ s.start, lastSlash + 1 }, contentPath.buf, contentPath.CAPACITY);

				if (!Eq(lastAutoRebasePath, contentPath))
				{
					lastAutoRebasePath = contentPath;
					database->SetContentPath(contentPath);

					sheets->SetContent(contentPath);
					return;
				}
			}
		}
	}

	WideFilePath fullPathCache;

	void UpdateAutoComplete(ISexyEditor& editor, const wchar_t* fullPath) override
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
		if (IsAlphaNumeric(activationChar) || activationChar == '.')
		{
			int64 displacementFromCaret = activationPoint - searchToken.start + 1;

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
					// Potentially a function call
					ShowFunctionArgumentsForType(editor, searchToken);	
				}
				else if (islower(*searchToken.start))
				{
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
		database->GetHintForCandidate(prefix, args);
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
	FactoryConfig config;

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
		ISexyStudioInstance1* ide = new SexyStudioIDE(topLevelParent, eventHandler, config);
		ShowWindow(ide->GetIDEFrame(), SW_HIDE);
		return ide;
	}

	void Free() override
	{
		delete this;
	}
};

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