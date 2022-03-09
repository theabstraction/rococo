// sexystudio.cpp : Defines the entry point for the application.
// Copyright (c) 2021 Mark Anthony Taylor

#include <rococo.libs.inl>
#include "sexystudio.impl.h"
#include "resource.h"
#include "rococo.auto-release.h"

#include <rococo.events.h>
#include <rococo.strings.h>

#include <uxtheme.h>
#pragma comment(lib, "uxtheme.lib")

#ifdef _DEBUG
# pragma comment(lib, "rococo.misc.utils.debug.lib")
#else
# pragma comment(lib, "rococo.misc.utils.lib")
#endif

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

using namespace Rococo;
using namespace Rococo::SexyStudio;
using namespace Rococo::Events;
using namespace Rococo::Sex;

auto evClose = "EvMainIDEWindowCloseRequest"_event;
auto evContentChange = "EvContentChange"_event; // TEventArg<cstr>
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
	void PopulateTreeWithSXYFiles(IGuiTree& tree, ISexyDatabase& database, IIDEFrame& frame, ISourceTree& sourceTree);
}

void OpenSexyFile(ISexyStudioEventHandler& evHandler, ISolution& solution, IWindow& mainWindow, cstr path, int lineNumber)
{
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
				Rococo::OS::ShellOpenDocument(packageFilepath);
			}
		}
		else
		{
			if (!evHandler.TryOpenEditor(path, lineNumber))
			{
				Rococo::OS::ShellOpenDocument(path);
			}
		}
	}
	catch (IException& ex)
	{
		Rococo::OS::ShowErrorBox(mainWindow, ex, "SexyStudio - Error");
	}
}

class PropertySheets: IObserver, IGuiTreeRenderer, IGuiTreeEvents
{
private:
	WidgetContext wc;
	IIDEFrame& ideFrame;
	ISexyDatabase& database;
	AutoFree<ISourceTree> idToSourceeMap = CreateSourceTree();
	IGuiTree* fileBrowser = nullptr;
	ITab* projectTab = nullptr;
	U8FilePath contentPath;

	void OnEvent(Event& ev) override
	{
		if (ev == evContentChange)
		{
			WaitCursorSection waitSection;
			ideFrame.SetProgress(0.0f, "Populating file browser...");

			database.Solution().SetContentFolder(contentPath);

			Rococo::OS::SetConfigVariable(contentPath, OS::ConfigSection{ "ContentPath" }, OS::ConfigRootName{ "SexyStudio" });

			PopulateTreeWithSXYFiles(*fileBrowser, database, ideFrame, *idToSourceeMap);
			ideFrame.SetProgress(100.0f, "Populated file browser");

			database.Sort();

			TEventArgs<ISexyDatabase*> args;
			args.value = &database;
			wc.publisher.Publish(args, evMetaUpdated);
		}
	}

	void RenderItem() override
	{

	}

public:
	PropertySheets(ISplitScreen& screen, IIDEFrame& _ideFrame, ISexyDatabase& _database): 
		wc(screen.Children()->Context()),
		ideFrame(_ideFrame),
		database(_database)
	{
		Format(contentPath, "%s", database.Solution().GetContentFolder());

		screen.SetBackgroundColour(RGBAb(128, 192, 128));

		ITabSplitter* tabs = CreateTabSplitter(*screen.Children());
		tabs->SetVisible(true);

		/*
		ITab& propTab = tabs->AddTab();
		propTab.SetName("Properties");
		propTab.SetTooltip("Property View");
		*/

		projectTab = &tabs->AddTab();
		projectTab->SetName("Projects");
		projectTab->SetTooltip("Project View");

		/*
		ITab& settingsTab = tabs->AddTab();
		settingsTab.SetName("Settings");
		settingsTab.SetTooltip("Global Configuration Settings");
		IVariableList* globalSettings = CreateVariableList(settingsTab.Children());
		globalSettings->SetVisible(true);

		Widgets::AnchorToParent(*globalSettings, 0, 0, 0, 0);
		*/

		IVariableList* projectSettings = CreateVariableList(projectTab->Children());
		projectSettings->SetVisible(true);

		Widgets::AnchorToParentTop(*projectSettings, 0);
		Widgets::AnchorToParentLeft(*projectSettings, 0);
		Widgets::AnchorToParentRight(*projectSettings, 0);
		Widgets::ExpandBottomFromTop(*projectSettings, 64);

		auto* contentEditor = projectSettings->AddFilePathEditor();
		contentEditor->SetName("Content");

		contentEditor->Bind(contentPath, 128);
		contentEditor->SetVisible(true);
		contentEditor->SetUpdateEvent(evContentChange);

		TreeStyle style;
		style.hasButtons = true;
		style.hasLines = true;

		fileBrowser = CreateTree(projectTab->Children(), style, *this, this);
		Widgets::AnchorToParent(*fileBrowser, 0, 64, 0, 0);

		fileBrowser->SetVisible(true);
		fileBrowser->SetImageList(4, IDB_FOLDER_CLOSED, IDB_FOLDER_OPEN, IDB_FILETYPE_SXY, IDB_FILETYPE_UNKNOWN);

		ideFrame.SetProgress(100.0f, "Complete!");

		wc.publisher.Subscribe(this, evContentChange);
	}

	~PropertySheets()
	{
		wc.publisher.Unsubscribe(this);
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
				Rococo::OS::ShowErrorBox(ideFrame.Window(), ex, "SexyStudio - Error");
			}
		}
	}

	void OnItemContextClick(IGuiTree& tree, ID_TREE_ITEM hItem, Vec2i pos) override
	{
		auto src = idToSourceeMap->Find(hItem);
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
			if (Eq(sDef[0].String()->Buffer, "class"))
			{
				if (IsAtomic(sDef[1]))
				{
					className = sDef[1].String()->Buffer;
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

				char desc[256];
				SafeFormat(desc, "extends %s", base);
				classTree->SetItemText(idBase, desc);
				classTree->SetItemImage(idBase, (int)ClassImageIndex::EXTENDS);
			}

			for (int j = 0; j < interf.AttributeCount(); ++j)
			{
				cstr attr = interf.GetAttribute(j);
				char desc[256];
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
		for (int i = 0; i < ns.TypeCount(); ++i)
		{
			auto& type = ns.GetType(i);
			auto idType = classTree->AppendItem(idNSNode);
			cstr publicName = type.PublicName();
			auto* localType = type.LocalType();

			idToType[idType] = &type;

			char desc[256];
			SafeFormat(desc, "%-64.64s %s", type.PublicName(), localType ? localType->SourcePath() : "");
			classTree->SetItemText(idType, desc);
			classTree->SetItemImage(idType, (int)ClassImageIndex::STRUCT);

			if (localType)
			{
				for (int j = 0; j < localType->FieldCount(); ++j)
				{
					auto idField = classTree->AppendItem(idType);

					auto field = localType->GetField(j);

					char desc[256];
					SafeFormat(desc, "%s %s", field.type, field.name);
					classTree->SetItemText(idField, desc);
					classTree->SetItemImage(idField, (int)ClassImageIndex::FIELD);
				}
			}
		}
	}

	std::unordered_map<ID_TREE_ITEM, ISXYArchetype*> idToArchetype;

	void AppendArchetypes(ISxyNamespace& ns, ID_TREE_ITEM idNSNode, ISexyDatabase& database, bool appendSourceName)
	{
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
				char desc[256];
				SafeFormat(desc, "%s %s", factory.InputType(k), factory.InputName(k));
				classTree->SetItemText(idInputArg, desc);
				classTree->SetItemImage(idInputArg, (int)ClassImageIndex::INPUT);
			}
		}
	}

	void AppendEnumerations(ISxyNamespace& ns, ID_TREE_ITEM idNSNode, ISexyDatabase& database)
	{
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

	void EnumerateNamesStartingWith(ISxyNamespace& ns, cstr prefix, IEventCallback<cstr>& cb)
	{
		for (int i = 0; i < ns.SubspaceCount(); ++i)
		{
			auto& subspace = ns[i];
			auto* name = ns[i].Name();
			if (StartsWith(name, prefix))
			{
				cb.OnEvent(name);
			}
		}

		for (int j = 0; j < ns.InterfaceCount(); ++j)
		{
			auto* name = ns.GetInterface(j).PublicName();
			if (StartsWith(name, prefix))
			{
				cb.OnEvent(name);
			}
		}

		for (int j = 0; j < ns.FunctionCount(); ++j)
		{
			auto* name = ns.GetFunction(j).PublicName();
			if (StartsWith(name, prefix))
			{
				cb.OnEvent(name);
			}
		}

		for (int j = 0; j < ns.FactoryCount(); ++j)
		{
			auto* name = ns.GetFactory(j).PublicName();
			if (StartsWith(name, prefix))
			{
				cb.OnEvent(name);
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
				auto& subpsace = ns[i];
				if (Eq(subpsace.Name(), subspaceName))
				{
					AppendToSearchTermsRecursive(subpsace, dot + 1, fullSearchItem);
					return;
				}
			}
		}
		else
		{
			for (int j = 0; j < ns.FunctionCount(); ++j)
			{
				auto& function = ns.GetFunction(j);
				auto* name = function.PublicName();
				if (StartsWith(name, searchTerm))
				{
					char fullName[256];
					strncpy_s(fullName, fullSearchItem, searchTerm - fullSearchItem);
					strncat_s(fullName, name, strlen(name));
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

	void OnMouseMovedInSearchBar(Vec2i pos)
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

	char searchTerms[256];
	IAsciiStringEditor* searchEditor = nullptr;
	IFloatingListWidget* searchResults = nullptr;
public:
	SexyExplorer(WidgetContext _wc, ISplitScreen& _screen, ISexyDatabase& _database, ISexyStudioEventHandler& _eventHandler) : 
		wc(_wc), screen(_screen), database(_database), eventHandler(_eventHandler)
	{
		screen.SetBackgroundColour(RGBAb(128, 128, 192));

		ITabSplitter* tabs = CreateTabSplitter(*screen.Children());
		tabs->SetVisible(true);

		classTab = &tabs->AddTab();
		classTab->SetName("Class View");

		IVariableList* searchBar = CreateVariableList(classTab->Children());
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
		classTree->SetImageList(15, IDB_BLANK, IDB_NAMESPACE, IDB_INTERFACE, IDB_METHOD, IDB_STRUCT, IDB_FIELD, IDB_EXTENDS, IDB_ATTRIBUTE, IDB_FUNCTION, IDB_INPUT, IDB_OUTPUT, IDB_ENUM, IDB_ALIAS, IDB_FACTORY, IDB_ARCHETYPE);

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

struct SexyStudioIDE: ISexyStudioInstance1, IObserver
{
	AutoFree<IPublisherSupervisor> publisher;
	AutoFree<ISexyDatabaseSupervisor> database;
	Font smallCaptionFont;
	WidgetContext context;
	AutoFree<ITheme> theme;
	AutoFree<IIDEFrameSupervisor> ide;
	ISplitScreen* splitScreen = nullptr;
	ISplitScreen* projectView = nullptr;
	ISplitScreen* sourceView = nullptr;

	PropertySheets* sheets = nullptr;
	SexyExplorer* explorer = nullptr;

	int64 autoCompleteCandidatePosition = 0;
	char callTipArgs[1024] = { 0 };

	std::vector<char> src_buffer;

	ISexyStudioEventHandler& eventHandler;

	void ReplaceCurrentSelectionWithCallTip(Rococo::AutoComplete::ISexyEditor& editor)
	{
		int64 caretPos = editor.GetCaretPos();

		if (*callTipArgs != 0 && autoCompleteCandidatePosition > 0 && autoCompleteCandidatePosition < caretPos)
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

	template<class ACTION> bool EnumerateFieldsOfClass(cstr className, cr_substring doc, ACTION& action)
	{
		cr_substring def = Rococo::Sexy::GetClassDefinition(className, doc);
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

			Rococo::Sexy::ForEachFieldOfClassDef(className, def, buildList);
			return true;
		}	

		return false;
	};

	struct RouteTextToAutoComplete: ISexyFieldEnumerator
	{
		IAutoCompleteBuilder& builder;
		cr_substring prefix;

		bool atLeastOneItem = false;

		HString hint;

		RouteTextToAutoComplete(IAutoCompleteBuilder& _builder, cr_substring _prefix):
			builder(_builder), prefix(_prefix)
		{

		}

		~RouteTextToAutoComplete()
		{
			if (atLeastOneItem)
			{
				builder.ShowAndClearItems();
			}
		}

		void OnField(cstr fieldName) override
		{
			atLeastOneItem = true;

			if (Length(prefix) > 0)
			{
				char prefixString[128];
				CopyWithTruncate(prefix, prefixString, sizeof prefixString);

				cstr separator = (prefix && prefix.end[-1] == '.') ? "" : ".";

				char item[256];
				SafeFormat(item, "%s%s%s", prefixString, separator, fieldName);
				builder.AddItem(item);
			}
			else
			{
				builder.AddItem(fieldName);
			}
		}

		void OnHintFound(cstr hintText) override
		{
			hint = hintText;
		}
	};

	void ShowAutocompleteDataForVariable(ISexyEditor& editor, cr_substring candidate, int64 tokenDisplacementFromCaret)
	{
		static auto thisDot = "this."_fstring;

		int64 caretPos = editor.GetCaretPos();

		RouteTextToAutoComplete routeTextToAutoComplete(editor.AutoCompleteBuilder(), candidate);

		int64 nCharsAndNull = editor.GetDocLength();
		src_buffer.resize(nCharsAndNull);
		editor.GetText(nCharsAndNull, src_buffer.data());

		Substring doc;
		doc.start = src_buffer.data();
		doc.end = doc.start + nCharsAndNull - 1;

		cstr docCaretPos = doc.start + caretPos;
		cstr start = docCaretPos - tokenDisplacementFromCaret;
		cstr end = start + Length(candidate);

		Substring candidateInDoc{ start, end };

		Substring variable = { candidateInDoc.start, candidateInDoc.end };

		U8FilePath contentFolder{ "C:\\content" };
		
		if (StartsWith(variable, thisDot))
		{
			variable.start += thisDot.length;
		}

		char type[256];
		bool isThis;
		if (Rococo::Sexy::TryGetLocalTypeFromCurrentDocument(type, isThis, candidateInDoc, doc))
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
					cstr finalType = routeTextToAutoComplete.hint.length() > 0 ? routeTextToAutoComplete.hint.c_str() : type;
					editor.ShowCallTipAtCaretPos(finalType);
				}
			}
			else
			{
				if (database->EnumerateVariableAndFieldList(variable, type, routeTextToAutoComplete))
				{

				}
				else
				{					
					cstr finalType = routeTextToAutoComplete.hint.length() > 0 ? routeTextToAutoComplete.hint.c_str() : type;
					editor.ShowCallTipAtCaretPos(finalType);
				}
			}
		}
	}

	void ShowAutocompleteDataForType(ISexyEditor& editor, cr_substring candidate)
	{
		Substring token = Rococo::Sexy::GetFirstTokenFromLeft(candidate);

		RouteTextToAutoComplete routeTextToAutoComplete(editor.AutoCompleteBuilder(), Substring_Null());
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
		Substring token = Rococo::Sexy::GetFirstTokenFromLeft(candidate);

		callTipArgs[0] = 0;

		GetHintForCandidate(token, callTipArgs);
		if (callTipArgs[0] != 0)
		{
			editor.ShowCallTipAtCaretPos(callTipArgs);
		}
	}

	bool TryAddTokenOptionsToAutocomplete(ISexyEditor& editor, cr_substring candidate, int64 displacementFromCaret)
	{
		using namespace Rococo;

		if (!candidate)
		{
			return false;
		}
		else if (Rococo::Sexy::IsSexyKeyword(candidate))
		{
			return false;
		}
		else if (islower(*candidate.start))
		{
			ShowAutocompleteDataForVariable(editor, candidate, displacementFromCaret);
			return true;
		}
		else if (isupper(*candidate.start))
		{
			ShowAutocompleteDataForType(editor, candidate);
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

	SexyStudioIDE(IWindow& topLevelWindow, ISexyStudioEventHandler& evHandler):
		publisher(Rococo::Events::CreatePublisher()),
		database(CreateSexyDatabase()),
		smallCaptionFont(MakeDefaultFont()),
		context{ *publisher, smallCaptionFont },
		theme{ UseNamedTheme("Classic", context.publisher) },
		eventHandler(evHandler)
	{
		ide = CreateMainIDEFrame(context, topLevelWindow, evHandler);
		Widgets::SetText(*ide, "Sexy Studio");
		Widgets::SetSpan(*ide, 1024, 600);

		splitScreen = CreateSplitScreen(ide->Children());
		Widgets::AnchorToParent(*splitScreen, 0, 0, 0, 0);

		splitScreen->SetBackgroundColour(RGBAb(192, 128, 128));
		splitScreen->SplitIntoColumns(400);

		projectView = splitScreen->GetFirstHalf();
		sourceView = splitScreen->GetSecondHalf();

		U8FilePath contentPath;
		Rococo::OS::GetConfigVariable(contentPath, "\\work\\rococo\\content\\", OS::ConfigSection{ "ContentPath" }, OS::ConfigRootName{ "SexyStudio" });

		database->SetContentPath(contentPath);

		sheets = new PropertySheets(*projectView, *ide, *database);
		explorer = new SexyExplorer(context, *sourceView, *database, eventHandler);

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
	}

	void ReplaceSelectedText(Rococo::AutoComplete::ISexyEditor& editor, cstr item)
	{
		int64 caretPos = editor.GetCaretPos();
		if (autoCompleteCandidatePosition > 0 && autoCompleteCandidatePosition < caretPos)
		{
			editor.ReplaceText(autoCompleteCandidatePosition, caretPos, item);
			UpdateAutoComplete(editor);
		}
	}

	void SetHintToFunctionArguments(Rococo::AutoComplete::ISexyEditor& editor, const ISXYFunction& f, bool appendCloseParenthesis = true)
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

	void UpdateAutoComplete(Rococo::AutoComplete::ISexyEditor& editor) override
	{
		EditorLine currentLine;
		if (!editor.TryGetCurrentLine(currentLine))
		{
			return;
		}

		EditorCursor cursor;
		editor.GetCursor(cursor);

		autoCompleteCandidatePosition = cursor.CaretPos();

		Substring substringLine{ currentLine.begin(), currentLine.end() };

		cstr endTokenPtr = substringLine.start + cursor.ColumnNumber();

		if (endTokenPtr > substringLine.start && (IsAlphaNumeric(endTokenPtr[-1]) || endTokenPtr[-1] == '.'))
		{
			autoCompleteCandidatePosition = cursor.CaretPos();

			cstr openingToken = Rococo::Sexy::GetFirstNonTokenPointerFromRight(substringLine, endTokenPtr);
			if (openingToken == nullptr)
			{
				openingToken = substringLine.start;
			}
			else
			{
				openingToken++; // this takes us into the alphanumeric string
			}
			
			Substring searchToken = Rococo::Sexy::GetFirstTokenFromLeft({ openingToken, substringLine.end });

			int64 displacementFromCaret = endTokenPtr - openingToken;

			if (TryAddTokenOptionsToAutocomplete(editor, searchToken, displacementFromCaret))
			{
				autoCompleteCandidatePosition = openingToken - substringLine.start + cursor.lineStartPosition;
			}
			else
			{
				autoCompleteCandidatePosition = 0;
			}
		}
		else if (endTokenPtr > substringLine.start && (endTokenPtr[-1] == ' ' || endTokenPtr[-1] == '\t'))
		{
			// Potentially we have a method or function call followed by a space, which is a prompt to show the function arguments
			endTokenPtr--;
			if (endTokenPtr > substringLine.start && IsAlphaNumeric(endTokenPtr[-1]))
			{
				autoCompleteCandidatePosition = cursor.CaretPos();

				cstr openingToken = Rococo::Sexy::GetFirstNonTokenPointerFromRight(substringLine, endTokenPtr);
				if (openingToken == nullptr)
				{
					openingToken = substringLine.start;
				}
				else
				{
					openingToken++; // this takes us into the alphanumeric string
				}
				
				Substring searchToken = Rococo::Sexy::GetFirstTokenFromLeft({ openingToken, substringLine.end });
				autoCompleteCandidatePosition = openingToken - substringLine.start + cursor.lineStartPosition;

				if (isupper(*openingToken))
				{
					// Potentially a function call
					ShowFunctionArgumentsForType(editor, searchToken);
					
				}
				else if (islower(*openingToken))
				{
					// Potentially a method call
					cstr separator = Rococo::ReverseFind('.', searchToken);
					if (separator && *separator == '.')
					{
						if (isupper(separator[1]))
						{
							// Potential method name, with left of separator being the interface variable
							char type[256];
							bool isThis;

							int64 nCharsAndNull = editor.GetDocLength();
							src_buffer.resize(nCharsAndNull);
							editor.GetText(nCharsAndNull, src_buffer.data());

							Substring doc;
							doc.start = src_buffer.data();
							doc.end = doc.start + nCharsAndNull - 1;

							int64 caretPos = editor.GetCaretPos();

							int64 displacementFromCaret = endTokenPtr - openingToken + 1;

							cstr docCaretPos = doc.start + caretPos;
							cstr start = docCaretPos - displacementFromCaret;
							cstr end = start + Length(searchToken);

							Substring candidateInDoc{ start, end};

							Substring methodName{ separator + 1, searchToken.end };

							if (Rococo::Sexy::TryGetLocalTypeFromCurrentDocument(type, isThis, candidateInDoc, doc))
							{
								auto* pInterface = database->FindInterface(type);
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
											break;
										}
									}
								}
							}
						}
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
		return new SexyStudioIDE(topLevelParent, eventHandler);
	}

	void Free() override
	{
		delete this;
	}
};

static bool isInitialized = false;

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

		Rococo::OS::SetBreakPoints(Rococo::OS::BreakFlag_All & ~Rococo::OS::BreakFlag_IllFormed_SExpression);

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