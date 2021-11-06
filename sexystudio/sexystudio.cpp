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


namespace Globals
{
	U8FilePath contentFolder{ "\\work\\rococo\\content\\scripts\\" };
	U8FilePath packageFolder{ "\\work\\rococo\\content\\packages\\mhost_1000.sxyz" };
	U8FilePath searchPath{ "" };
}

namespace Rococo::SexyStudio
{
	void PopulateTreeWithSXYFiles(IGuiTree& tree, cstr contentFolder, ISexyDatabase& database, IIDEFrame& frame);
}

class PropertySheets: IObserver, IGuiTreeRenderer
{
private:
	WidgetContext wc;
	IIDEFrame& ideFrame;
	ISexyDatabase& database;
	IGuiTree* fileBrowser = nullptr;
	ITab* projectTab = nullptr;

	void OnEvent(Event& ev) override
	{
		if (ev == evContentChange)
		{
			WaitCursorSection waitSection;
			ideFrame.SetProgress(0.0f, "Populating file browser...");
			PopulateTreeWithSXYFiles(*fileBrowser, Globals::contentFolder, database, ideFrame);
			PopulateTreeWithPackages(Globals::searchPath, Globals::packageFolder, database);
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
		contentEditor->Bind(Globals::contentFolder, 128);
		contentEditor->SetVisible(true);
		contentEditor->SetUpdateEvent(evContentChange);

		auto* packagePathEditor = projectSettings->AddFilePathEditor();
		packagePathEditor->SetName("Package Path");
		packagePathEditor->Bind(Globals::packageFolder, 128);
		packagePathEditor->SetVisible(true);
		packagePathEditor->SetUpdateEvent(evContentChange);

		TreeStyle style;
		style.hasButtons = true;
		style.hasLines = true;

		fileBrowser = CreateTree(projectTab->Children(), style, this);
		Widgets::AnchorToParent(*fileBrowser, 0, 64, 0, 0);

		fileBrowser->SetVisible(true);
		fileBrowser->SetImageList(4, IDB_FOLDER_CLOSED, IDB_FOLDER_OPEN, IDB_FILETYPE_SXY, IDB_FILETYPE_UNKNOWN);

	//	PopulateTreeWithSXYFiles(*fileBrowser, Globals::contentFolder, *database, ideFrame);

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

class SexyExplorer: IObserver
{
private:
	WidgetContext wc;
	ISplitScreen& screen;
	ISexyDatabase& database;
	IGuiTree* classTree;
	ITab* classTab = nullptr;

	void AppendArguments(ID_TREE_ITEM idFunction, ISXYArchetype& archetype)
	{
		if (archetype.InputCount() + archetype.OutputCount() == 0)
		{
			auto idNoArg = classTree->AppendItem(idFunction);
			classTree->SetItemText(" - no arguments -", idNoArg);
			classTree->SetItemImage(idNoArg, (int)ClassImageIndex::BLANK);
		}

		for (int k = 0; k < archetype.InputCount(); ++k)
		{
			auto idInputArg = classTree->AppendItem(idFunction);
			char desc[256];
			SafeFormat(desc, "%s %s", archetype.InputType(k), archetype.InputName(k));
			classTree->SetItemText(desc, idInputArg);
			classTree->SetItemImage(idInputArg, (int)ClassImageIndex::INPUT);
		}

		if (archetype.OutputCount() > 0)
		{
			auto idMapArg = classTree->AppendItem(idFunction);
			classTree->SetItemText("->", idMapArg);
			classTree->SetItemImage(idMapArg, (int)ClassImageIndex::BLANK);

			for (int k = 0; k < archetype.OutputCount(); ++k)
			{
				auto idOutputArg = classTree->AppendItem(idFunction);
				char desc[256];
				SafeFormat(desc, "%s %s", archetype.OutputType(k), archetype.OutputName(k));
				classTree->SetItemText(desc, idOutputArg);
				classTree->SetItemImage(idOutputArg, (int)ClassImageIndex::OUTPUT);
			}
		}
	}

	void AppendInterfaces(ISxyNamespace& ns, ID_TREE_ITEM idNSNode)
	{
		for (int i = 0; i < ns.InterfaceCount(); ++i)
		{
			auto& interf = ns.GetInterface(i);
			auto idInterface = classTree->AppendItem(idNSNode);

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

			classTree->SetItemText(desc, idInterface);
			classTree->SetItemImage(idInterface, (int)ClassImageIndex::INTERFACE);

			cstr base = interf.Base();
			if (base)
			{
				auto idBase = classTree->AppendItem(idInterface);

				char desc[256];
				SafeFormat(desc, "extends %s", base);
				classTree->SetItemText(desc, idBase);
				classTree->SetItemImage(idBase, (int)ClassImageIndex::EXTENDS);
			}

			for (int j = 0; j < interf.AttributeCount(); ++j)
			{
				cstr attr = interf.GetAttribute(j);
				char desc[256];
				SafeFormat(desc, "attribute %s", attr);
				auto idAttr = classTree->AppendItem(idInterface);
				classTree->SetItemText(desc, idAttr);
				classTree->SetItemImage(idAttr, (int)ClassImageIndex::ATTRIBUTE);
			}

			for (int j = 0; j < interf.MethodCount(); ++j)
			{
				auto& method = interf.GetMethod(j);

				auto idMethod = classTree->AppendItem(idInterface);
				classTree->SetItemText(method.PublicName(), idMethod);
				classTree->SetItemImage(idMethod, (int)ClassImageIndex::METHOD);

				AppendArguments(idMethod, method);
			}
		}
	}

	void AppendTypes(ISxyNamespace& ns, ID_TREE_ITEM idNSNode, ISexyDatabase& database)
	{
		for (int i = 0; i < ns.TypeCount(); ++i)
		{
			auto& type = ns.GetType(i);
			auto idType = classTree->AppendItem(idNSNode);
			cstr publicName = type.PublicName();
			auto* localType = type.LocalType();

			char desc[256];
			SafeFormat(desc, "%-64.64s %s", type.PublicName(), localType ? localType->SourcePath() : "");
			classTree->SetItemText(desc, idType);
			classTree->SetItemImage(idType, (int)ClassImageIndex::STRUCT);

			if (localType)
			{
				for (int j = 0; j < localType->FieldCount(); ++j)
				{
					auto idField = classTree->AppendItem(idType);

					auto field = localType->GetField(j);

					char desc[256];
					SafeFormat(desc, "%s %s", field.type, field.name);
					classTree->SetItemText(desc, idField);
					classTree->SetItemImage(idField, (int)ClassImageIndex::FIELD);
				}
			}
		}
	}

	void AppendArchetypes(ISxyNamespace& ns, ID_TREE_ITEM idNSNode, ISexyDatabase& database, bool appendSourceName)
	{
		for (int i = 0; i < ns.ArchetypeCount(); ++i)
		{
			auto& archetype = ns.GetArchetype(i);
			auto idArchetype = classTree->AppendItem(idNSNode);
			cstr publicName = archetype.PublicName();

			char desc[256];
			SafeFormat(desc, "%-64.64s %s", publicName, appendSourceName ? archetype.SourcePath() : "");
			classTree->SetItemText(desc, idArchetype);
			classTree->SetItemImage(idArchetype, (int)ClassImageIndex::ARCHETYPE);

			AppendArguments(idArchetype, archetype);
		}
	}

	std::unordered_map<ISXYPublicFunction*, ID_TREE_ITEM> mapPublicFunctionToTreeItem;

	void MapFunctionToClassTree(ISXYPublicFunction& function, ID_TREE_ITEM itemId)
	{
		mapPublicFunctionToTreeItem[&function] = itemId;
	}

	void AppendFunctions(ISxyNamespace& ns, ID_TREE_ITEM idNSNode, ISexyDatabase& database, bool appendSourceName)
	{
		for (int i = 0; i < ns.FunctionCount(); ++i)
		{
			auto& function = ns.GetFunction(i);
			auto idFunction = classTree->AppendItem(idNSNode);
			cstr publicName = function.PublicName();
			auto* localFunction = function.LocalFunction();

			MapFunctionToClassTree(function, idFunction);

			char desc[256];
			SafeFormat(desc, "%-64.64s %s", publicName, appendSourceName && localFunction ? localFunction->SourcePath() : "");
			classTree->SetItemText(desc, idFunction);
			classTree->SetItemImage(idFunction, (int)ClassImageIndex::FUNCTION);

			if (localFunction)
			{
				AppendArguments(idFunction, *localFunction);
			}
		}
	}

	void AppendFactories(ISxyNamespace& ns, ID_TREE_ITEM idNSNode, ISexyDatabase& database, bool appendSourceName)
	{
		for (int i = 0; i < ns.FactoryCount(); ++i)
		{
			auto& factory = ns.GetFactory(i);
			auto idFactory = classTree->AppendItem(idNSNode);
			cstr publicName = factory.PublicName();

			char definedInterface[256];
			factory.GetDefinedInterface(definedInterface, sizeof definedInterface);

			char shortDesc[256];
			SafeFormat(shortDesc, "%-24.24s --> (%s)", publicName, definedInterface);

			char desc[256];
			SafeFormat(desc, "%-64.64s %s", shortDesc, appendSourceName  ? factory.SourcePath() : "");
			classTree->SetItemText(desc, idFactory);
			classTree->SetItemImage(idFactory, (int)ClassImageIndex::FACTORY);

			if (factory.InputCount()  == 0)
			{
				auto idNoArg = classTree->AppendItem(idFactory);
				classTree->SetItemText(" - no arguments -", idNoArg);
				classTree->SetItemImage(idNoArg, (int)ClassImageIndex::BLANK);
			}

			for (int k = 0; k < factory.InputCount(); ++k)
			{
				auto idInputArg = classTree->AppendItem(idFactory);
				char desc[256];
				SafeFormat(desc, "%s %s", factory.InputType(k), factory.InputName(k));
				classTree->SetItemText(desc, idInputArg);
				classTree->SetItemImage(idInputArg, (int)ClassImageIndex::INPUT);
			}
		}
	}

	void AppendEnumerations(ISxyNamespace& ns, ID_TREE_ITEM idNSNode, ISexyDatabase& database)
	{
		if (ns.EnumCount() > 0)
		{
			auto idEnums = classTree->AppendItem(idNSNode);
			classTree->SetItemText("Enumerations", idEnums);
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
				classTree->SetItemText(fulldesc, idEnum);
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
			classTree->SetItemText(fulldesc, idAlias);
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
			classTree->SetItemText(subspace.Name(), idBranch);
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
		classTree->SetItemText("Namespaces", idNamespace);
		classTree->SetItemImage(idNamespace, (int) ClassImageIndex::NAMESPACE);

		mapNSToTreeItemId.clear();
		mapTreeItemIdToNS.clear();
		mapPublicFunctionToTreeItem.clear();

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

		if (searchArrayResults.empty() && *Globals::searchPath == 0)
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
	SexyExplorer(WidgetContext _wc, ISplitScreen& _screen, ISexyDatabase& _database) : wc(_wc), screen(_screen), database(_database)
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
		classTree = CreateTree(classTab->Children(), style);
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

static std::vector<fstring> keywords
{
	"method"_fstring, "function"_fstring, "class"_fstring, "struct"_fstring
};

static bool IsEndOfToken(char c)
{
	switch (c)
	{
	case '(':
	case ')':
	case ' ':
	case '\r':
	case '\n':
	case '\t':
		return true;
	}

	return false;
}

cstr GetFirstNonTokenPointer(substring_ref s)
{
	for (cstr p = s.start; p < s.end; ++p)
	{
		if (!IsAlphaNumeric(*p) && *p != '.')
		{
			return p;
		}
	}

	return s.end;
}

cstr GetFirstNonTypeCharPointer(substring_ref s)
{
	bool inDot = false;

	for (cstr p = s.start; p < s.end; ++p)
	{
		if (!inDot)
		{
			if (*p == '.')
			{
				inDot = true;
				continue;
			}
		}

		if (IsAlphaNumeric(*p))
		{
			if (inDot)
			{
				inDot = false;
			}

			continue;
		}

		return p;
	}

	return s.end;
}

thread_local AutoFree<IDynamicStringBuilder> dsb = CreateDynamicStringBuilder(1024);

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

	void ReplaceCurrentSelectionWithCallTip(Rococo::AutoComplete::ISexyEditor& editor)
	{
		int64 caretPos = editor.GetCaretPos();

		if (*callTipArgs != 0 && autoCompleteCandidatePosition > 0 && autoCompleteCandidatePosition < caretPos)
		{
			editor.ReplaceText(caretPos, caretPos, callTipArgs);
			callTipArgs[0] = 0;
		}
	}

	bool TryGetType(ISexyEditor& editor, substring_ref candidate, char type[256], char name[256], ptrdiff_t caretPos)
	{
		if (caretPos <= 0 || candidate.start == nullptr || !islower(*candidate.start))
		{
			return false;
		}

		src_buffer.resize(caretPos + 1);

		Substring token = { candidate.start, GetFirstNonTokenPointer(candidate) };

		editor.GetText(caretPos + 1, src_buffer.data());

		cstr end = src_buffer.data() + caretPos;

		using namespace Rococo::Sexy;
		BadlyFormattedTypeInferenceEngine engine(src_buffer.data());

		cstr start = end - Length(token);

		if (end - start > 1 && end[-1] == '.') end--; // Hide terminating dot

		Substring searchTerm{ start, end };

		static auto thisDot = "this."_fstring;

		auto inference = engine.InferParentVariableType(searchTerm);
		if (inference.declarationType)
		{
			if (StartsWith(searchTerm, thisDot) && Length(searchTerm) > thisDot.length)
			{
				// We inferred the parent type of the member variable
				Substring parentNameAndChildren = RightOfFirstChar('.', searchTerm);
				searchTerm = RightOfFirstChar('.', parentNameAndChildren);
			}

			TypeInferenceType tit;
			engine.GetType(tit, inference);
			SafeFormat(type, 256, "%s", tit.buf);
			return SubstringToString(name, 256, searchTerm);
		}
		else
		{
			*type = 0;
			return false;
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

	void EnumerateFieldsOfClass(cstr prefix, cstr className, IEnumerator<cstr>& cb, ISexyEditor& editor)
	{
		int64 len = editor.GetDocLength();
		if (len < 10 || len >(int64)1_megabytes)
		{
			return;
		}

		std::vector<char> docBuffer;
		docBuffer.resize(len + 1);
		editor.GetText(len + 1, docBuffer.data());

		Substring doc{ docBuffer.data(), docBuffer.data() + docBuffer.size() };

		substring_ref def = Rococo::Sexy::GetClassDefinition(className, doc);
		if (def)
		{
			struct ANON: IFieldEnumerator
			{
				cstr prefix;
				IEnumerator<cstr>& cb;

				void OnMemberVariable(cstr name, cstr type) override
				{
					if (Eq(type, "implements"_fstring))
					{

					}
					else
					{
						char publishName[128];
						SafeFormat(publishName, "%s%s", prefix, name);
						cb(publishName);
					}
				}

				ANON(IEnumerator<cstr>& _cb, cstr _prefix) : cb(_cb), prefix(_prefix) {}
			} copyWithPrefix(cb, prefix);

			Rococo::Sexy::ForEachFieldOfClassDef(className, def, copyWithPrefix);
		}	
	};

	void ShowAutocompleteDataForVariable(ISexyEditor& editor, substring_ref candidate)
	{
		static auto thisDot = "this."_fstring;

		int64 caretPos = editor.GetCaretPos();

		char type[256];
		char name[256];
		if (TryGetType(editor, candidate, type, name, caretPos))
		{
			auto& sb = dsb->Builder();
			sb.Clear();

			SpaceSeparatedStringItems appendToString(sb);

			if (Eq(name, "this"))
			{
				EnumerateFieldsOfClass("this.", type, appendToString, editor);
				if (sb.Length() > 0)
				{
					editor.ShowAutoCompleteList(*sb);
				}
				else
				{
					editor.ShowCallTipAtCaretPos(type);
				}
			}
			else if (StartsWith(candidate, thisDot))
			{
				Substring thisSubstring{ candidate.start, candidate.start + 5 };
				if (database->EnumerateVariableAndFieldList(thisSubstring, name, type, appendToString))
				{
					editor.ShowAutoCompleteList(*sb);
				}
			}
			else if (database->EnumerateVariableAndFieldList(candidate, name, type, appendToString))
			{
				editor.ShowAutoCompleteList(*sb);
			}
			else
			{
				editor.ShowCallTipAtCaretPos(type);
			}
		}
	}

	void ShowAutocompleteDataForType(ISexyEditor& editor, substring_ref candidate)
	{
		Substring token = { candidate.start, GetFirstNonTypeCharPointer(candidate) };

		auto& sb = dsb->Builder();
		sb.Clear();

		struct ANON : IEnumerator<cstr>
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

			ANON(StringBuilder& _sb) : sb(_sb) {}
		} appendToString(sb);
		ForEachAutoCompleteCandidate(token, appendToString);

		const fstring& sbString = *dsb->Builder();

		callTipArgs[0] = 0;

		if (appendToString.count == 1)
		{
			GetHintForCandidate(token, callTipArgs);
			if (callTipArgs[0] != 0)
			{
				editor.ShowCallTipAtCaretPos(callTipArgs);
				return;
			}
		}

		if (appendToString.count > 0)
		{
			editor.SetAutoCompleteCancelWhenCaretMoved();
			editor.ShowAutoCompleteList(sbString.buffer);
		}
	}

	bool TryParseToken(ISexyEditor& editor, substring_ref candidate)
	{
		using namespace Rococo;

		size_t len = candidate.end - candidate.start;

		for (auto keyword : keywords)
		{
			if (StartsWith(candidate, keyword))
			{
				if (len > keyword.length && IsEndOfToken(candidate.start[keyword.length]))
				{
					// We found a keyword, but we do not need to parse it
					return false;
				}
			}
		}

		if (islower(*candidate.start))
		{
			ShowAutocompleteDataForVariable(editor, candidate);
			return true;
		}
		else if (isupper(*candidate.start) && len > 2)
		{
			ShowAutocompleteDataForType(editor, candidate);
			return true;
		}

		return false;
	}

	SexyStudioIDE(IWindow& topLevelWindow):
		publisher(Rococo::Events::CreatePublisher()),
		database(CreateSexyDatabase()),
		smallCaptionFont(MakeDefaultFont()),
		context{ *publisher, smallCaptionFont },
		theme{ UseNamedTheme("Classic", context.publisher) }
	{
		ide = CreateMainIDEFrame(context, topLevelWindow);
		Widgets::SetText(*ide, "Sexy Studio");
		Widgets::SetSpan(*ide, 1024, 600);

		splitScreen = CreateSplitScreen(ide->Children());
		Widgets::AnchorToParent(*splitScreen, 0, 0, 0, 0);

		splitScreen->SetBackgroundColour(RGBAb(192, 128, 128));
		splitScreen->SplitIntoColumns(400);

		projectView = splitScreen->GetFirstHalf();
		sourceView = splitScreen->GetSecondHalf();

		sheets = new PropertySheets(*projectView, *ide, *database);
		explorer = new SexyExplorer(context, *sourceView, *database);

		publisher->Subscribe(this, evIDEClose);
		publisher->Subscribe(this, evIDEMax);
		publisher->Subscribe(this, evIDEMin);

		ide->SetVisible(true);
		splitScreen->SetVisible(true);
		ide->LayoutChildren();

		sheets->CollapseTree();

		AutoFree<IDynamicStringBuilder> heapStringBuilder = CreateDynamicStringBuilder(1024);
		auto& sb = heapStringBuilder->Builder();
		Rococo::SexyStudio::AppendDescendantsAndRectsToString(*ide, sb);
		puts(*sb);

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

		cstr lastCandidateInLine = currentLine.end();

		for (cstr p = currentLine.end(); p >= currentLine.begin(); p--)
		{
			switch (*p)
			{
			case '(':
			{
				cstr candidate = p + 1;
				if (candidate == lastCandidateInLine)
				{
					return;
				}
				else if (TryParseToken(editor, { candidate,  lastCandidateInLine }))
				{
					auto candidateColumn = candidate - currentLine.begin();
					lastCandidateInLine = currentLine.begin() + candidateColumn;
					autoCompleteCandidatePosition = cursor.LineStartPosition() + candidateColumn;
					return;
				}
			}
			}
		}

		autoCompleteCandidatePosition = 0;
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

	void ForEachAutoCompleteCandidate(substring_ref prefix, IEnumerator<cstr>& action) override
	{
		database->ForEachAutoCompleteCandidate(prefix, action);
	}

	void GetHintForCandidate(substring_ref prefix, char args[1024]) override
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

cstr URL_base = "Rococo.SexyStudio.ISexyStudioBase";
cstr URL_factory = "Rococo.SexyStudio.ISexyStudioFactory1";

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

	ISexyStudioInstance1* CreateSexyIDE(IWindow& topLevelParent) override
	{
		return new SexyStudioIDE(topLevelParent);
	}

	void Free() override
	{
		delete this;
	}
};

static bool isInitialized = false;

extern "C" _declspec(dllexport) int CreateSexyStudioFactory(void** ppInterface, const char* interfaceURL)
{
	if (!isInitialized)
	{
		HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
		if (FAILED(hr))
		{
			return hr;
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

	if (ppInterface == nullptr || interfaceURL == nullptr)
	{
		return E_POINTER;
	}

	if (Eq(interfaceURL, URL_base) || Eq(interfaceURL, URL_factory))
	{
		*ppInterface = (void*) new Factory();
		return S_OK;
	}

	return E_NOTIMPL;
}