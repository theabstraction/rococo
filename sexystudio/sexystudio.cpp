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

#include <vector>
#include <string>

using namespace Rococo;
using namespace Rococo::SexyStudio;
using namespace Rococo::Events;
using namespace Rococo::Sex;

auto evClose = "EvMainIDEWindowCloseRequest"_event;
auto evContentChange = "EvContentChange"_event; // TEventArg<cstr>

ROCOCOAPI IMessagePump
{
	virtual void MainLoop() = 0;
	virtual void Quit() = 0;
};

struct Win32MessagePump : IMessagePump
{
	void MainLoop() override
	{
		MSG msg;
		while (GetMessage(&msg, nullptr, 0, 0))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			AssertNoMessageError();
		}
	}

	void Quit() override
	{
		PostQuitMessage(0);
	}
};

auto evIDEClose = "EvIDEClose"_event;
auto evIDEMin = "EvIDEMin"_event;
auto evIDEMax = "EvIDEMax"_event;

namespace Globals
{
	U8FilePath contentFolder{ "\\work\\rococo\\content\\scripts" };
}

namespace Rococo::SexyStudio
{
	void PopulateTreeWithSXYFiles(IGuiTree& tree, cstr contentFolder, ISXYMetaTable& metaTable, IIDEFrame& frame);
}

auto evMetaUpdated = "sexystudio.meta.updated"_event;

class PropertySheets: IObserver, IGuiTreeRenderer
{
private:
	WidgetContext wc;
	IIDEFrame& ideFrame;
	IGuiTree* fileBrowser = nullptr;
	AutoFree<ISXYMetaTable> metaTable;

	void OnEvent(Event& ev) override
	{
		if (ev == evContentChange)
		{
			WaitCursorSection waitSection;
			ideFrame.SetProgress(0.0f, "Populating file browser...");
			PopulateTreeWithSXYFiles(*fileBrowser, Globals::contentFolder, *metaTable, ideFrame);
			ideFrame.SetProgress(100.0f, "Populated file browser");

			TEventArgs<ISXYMetaTable*> args;
			args.value = metaTable;
			wc.publisher.Publish(args, evMetaUpdated);
		}
	}

	void RenderItem() override
	{

	}

	int GetExpandedImageIndex(uint64 contextId) const override
	{
		return 1; // IDB_FOLDER_OPEN
	}

	int GetContractedImageIndex(uint64 contextId) const  override
	{
		return 0; // IDB_FOLDER_CLOSED
	}

public:
	PropertySheets(ISplitScreen& screen, IIDEFrame& _ideFrame): 
		wc(screen.Children()->Context()),
		ideFrame(_ideFrame),
		metaTable(CreateSXYMetaTable())
	{
		screen.SetBackgroundColour(RGBAb(128, 192, 128));

		ITabSplitter* tabs = CreateTabSplitter(*screen.Children());
		tabs->SetVisible(true);

		/*
		ITab& propTab = tabs->AddTab();
		propTab.SetName("Properties");
		propTab.SetTooltip("Property View");
		*/

		ITab& projectTab = tabs->AddTab();
		projectTab.SetName("Projects");
		projectTab.SetTooltip("Project View");

		/*
		ITab& settingsTab = tabs->AddTab();
		settingsTab.SetName("Settings");
		settingsTab.SetTooltip("Global Configuration Settings");
		IVariableList* globalSettings = CreateVariableList(settingsTab.Children());
		globalSettings->SetVisible(true);

		Widgets::AnchorToParent(*globalSettings, 0, 0, 0, 0);
		*/

		IVariableList* projectSettings = CreateVariableList(projectTab.Children());
		projectSettings->SetVisible(true);

		Widgets::AnchorToParentTop(*projectSettings, 0);
		Widgets::AnchorToParentLeft(*projectSettings, 0);
		Widgets::AnchorToParentRight(*projectSettings, 0);
		Widgets::ExpandBottomFromTop(*projectSettings, 32);

		auto* contentEditor = projectSettings->AddFilePathEditor();
		contentEditor->SetName("Content");
		contentEditor->Bind(Globals::contentFolder, 128);
		contentEditor->SetVisible(true);
		contentEditor->SetUpdateEvent(evContentChange);

		TreeStyle style;
		style.hasButtons = true;
		style.hasLines = true;

		fileBrowser = CreateTree(projectTab.Children(), style, this);
		Widgets::AnchorToParent(*fileBrowser, 0, 32, 0, 0);

		fileBrowser->SetVisible(true);
		fileBrowser->SetImageList(4, IDB_FOLDER_CLOSED, IDB_FOLDER_OPEN, IDB_FILETYPE_SXY, IDB_FILETYPE_UNKNOWN);

		Widgets::AnchorToParent(*fileBrowser, 0, 32, 0, 0);

		PopulateTreeWithSXYFiles(*fileBrowser, Globals::contentFolder, *metaTable, ideFrame);

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

	ISXYMetaTable& Meta() { return *metaTable;  }
};

struct TAtomicArg
{
	bool Matches(cr_sex s)
	{
		return IsAtomic(s);
	}

	fstring operator()(cr_sex s)
	{
		return fstring{ s.String()->Buffer, s.String()->Length };
	}
};

struct ParseKeyword
{
	fstring keyword;
	ParseKeyword(cstr _keyword) : keyword(to_fstring(_keyword))
	{
	}

	bool Matches(cr_sex s)
	{
		if (!IsAtomic(s)) return false;
		return Eq(s.String()->Buffer, keyword);
	}

	fstring operator()(cr_sex s)
	{
		return keyword;
	}
};

ParseKeyword keywordNamespace("namespace");
ParseKeyword keywordInterface("interface");
ParseKeyword keywordStruct("struct");
ParseKeyword keywordFunction("function");
ParseKeyword keywordMacro("macro");
ParseKeyword keywordAlias("alias");
TAtomicArg ParseAtomic;

template<class ACTION, class FIRSTARG, class SECONDARG>
bool match_compound(cr_sex s, int nMaxArgs, FIRSTARG a, SECONDARG b, ACTION action)
{
	if (!IsCompound(s)) return false;
	if (s.NumberOfElements() < 2) return false;
	if (s.NumberOfElements() > nMaxArgs) return false;

	if (!a.Matches(s[0])) return false;
	if (!b.Matches(s[1])) return false;

	action(s, a(s[0]), b(s[1]));

	return true;
}

template<class ACTION, class FIRSTARG, class SECONDARG, class THIRDARG>
bool match_compound(cr_sex s, int nMaxArgs, FIRSTARG a, SECONDARG b, THIRDARG c, ACTION action)
{
	if (!IsCompound(s)) return false;
	if (s.NumberOfElements() < 3) return false;
	if (s.NumberOfElements() > nMaxArgs) return false;

	if (!a.Matches(s[0])) return false;
	if (!b.Matches(s[1])) return false;
	if (!c.Matches(s[1])) return false;

	action(s, a(s[0]), b(s[1]), c(s[2]));

	return true;
}

struct TreeManager
{
	struct ItemDesc
	{
		enum class Type: int32 { NAMESPACE, OTHER };
		Type type;
		std::string name;
	};

	std::vector<ItemDesc*> items;

	~TreeManager()
	{
		for (ItemDesc* i : items)
		{
			delete i;
		}
	}

	void AddNS(ID_TREE_ITEM id, IGuiTree& tree, cstr desc)
	{
		ItemDesc* i = new ItemDesc;
		i->name = desc;
		i->type = ItemDesc::Type::NAMESPACE;
		tree.SetContext(id, (uint64) i);
	}

	int Compare(LPARAM lParam1, LPARAM lParam2)
	{
		if (lParam1 == 0 && lParam2 == 0) return 0;
		if (lParam1 == 0) return 1;
		if (lParam2 == 0) return -1;

		ItemDesc* desc1 = (ItemDesc*)lParam1;
		ItemDesc* desc2 = (ItemDesc*)lParam2;

		return strcmp(desc1->name.c_str(), desc2->name.c_str());
	}

	ID_TREE_ITEM GetSubspace(IGuiTree& tree, ID_TREE_ITEM idCurrentNS, cstr subspace);
	void InsertNamespaceUnique(IGuiTree& tree, ID_TREE_ITEM idNamespace, cstr namespaceText);
	void InsertInterface(IGuiTree& tree, ID_TREE_ITEM idNamespace, cstr interfaceName, cr_sex sInterfaceDef);
	void InsertAlias(IGuiTree& tree, ID_TREE_ITEM idNamespace, cstr aliasFrom, cstr aliasTo, cr_sex sAliasDef);
	void InsertMacro(IGuiTree& tree, ID_TREE_ITEM idNamespace, cstr macroName, cr_sex sMacroDef);
	void InsertStruct(IGuiTree& tree, ID_TREE_ITEM idTypes, cstr structName, cr_sex sStructDef);
	void InsertFunction(IGuiTree& tree, ID_TREE_ITEM idFunction, cstr fnName, cr_sex sFunctionDef);
	void BuildNamespaces(IGuiTree& tree, ID_TREE_ITEM idNamespace, ID_TREE_ITEM idTypes, ID_TREE_ITEM idFunctions, cr_sex sRoot);
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

void TreeManager::InsertAlias(IGuiTree& tree, ID_TREE_ITEM idNamespace, cstr aliasFrom, cstr aliasTo, cr_sex sAliasDef)
{
	cstr s;
	for (s = aliasTo; *s != '.' && *s != 0; s++)
	{
	}

	char* subspace = (char*)_alloca(s - aliasTo + 1);
	memcpy(subspace, aliasTo, s - aliasTo);
	subspace[s - aliasTo] = 0;

	if (*s == 0)
	{
		// This is the interface short name rather than the subspace
		auto idAlias = tree.AppendItem(idNamespace);

		if (IsDotted(aliasFrom))
		{
			char srcDesc[256];
			SafeFormat(srcDesc, 256, "alias %s as %s", aliasFrom, subspace);

			char desc[256];
			SafeFormat(desc, 256, "%-64s %s", srcDesc, sAliasDef.Tree().Source().Name());
			tree.SetItemText(desc, idAlias);
		}
		else
		{
			char srcDesc[256];
			SafeFormat(srcDesc, 256, "alias local %s as %s", aliasFrom, subspace);

			char desc[256];
			SafeFormat(desc, 256, "%s: %s", srcDesc, sAliasDef.Tree().Source().Name());
			tree.SetItemText(desc, idAlias);
		}

		tree.SetItemImage(idAlias, 12);
	}
	else
	{
		ID_TREE_ITEM idSubspace = GetSubspace(tree, idNamespace, subspace);
		InsertAlias(tree, idSubspace, aliasFrom, s + 1, sAliasDef);
	}
}

ID_TREE_ITEM TreeManager::GetSubspace(IGuiTree& tree, ID_TREE_ITEM idCurrentNS, cstr subspace)
{
	struct ANON: IEventCallback<TreeItemInfo>
	{
		IGuiTree* tree;
		ID_TREE_ITEM idBranch = 0;
		cstr subspace;
		void OnEvent(TreeItemInfo& info) override
		{
			char branch[256];
			tree->GetText(branch, sizeof branch, info.idItem);

			if (Eq(branch, subspace))
			{
				idBranch = info.idItem;
			}
		}
	} findMatchingSubspace;
	findMatchingSubspace.tree = &tree;
	findMatchingSubspace.subspace = subspace;
	tree.EnumerateChildren(idCurrentNS, findMatchingSubspace);

	if (findMatchingSubspace.idBranch == 0)
	{
		auto idNewItem = tree.AppendItem(idCurrentNS);
		tree.SetItemText(subspace, idNewItem);
		tree.SetItemImage(idNewItem, 1);
		AddNS(idNewItem, tree, subspace);
		return idNewItem;
	}
	else
	{
		return findMatchingSubspace.idBranch;
	}
}

void TreeManager::InsertNamespaceUnique(IGuiTree& tree, ID_TREE_ITEM idNamespace, cstr namespaceText)
{
	cstr s;
	for (s = namespaceText; *s != '.' && *s != 0; s++)
	{
	}

	char* subspace = (char*)_alloca(s - namespaceText + 1);
	memcpy(subspace, namespaceText, s - namespaceText);
	subspace[s - namespaceText] = 0;

	ID_TREE_ITEM idSubspace = GetSubspace(tree, idNamespace, subspace);

	if (*s != 0)
	{
		InsertNamespaceUnique(tree, idSubspace, s + 1);
	}
}

cstr AlwaysGetAtomic(cr_sex s)
{
	return IsAtomic(s) ? s.String()->Buffer : "<expected atomic argument>";
}

void TreeManager::InsertInterface(IGuiTree& tree, ID_TREE_ITEM idNamespace, cstr interfaceName, cr_sex sInterfaceDef)
{
	cstr s;
	for (s = interfaceName; *s != '.' && *s != 0; s++)
	{
	}

	char* subspace = (char*)_alloca(s - interfaceName + 1);
	memcpy(subspace, interfaceName, s - interfaceName);
	subspace[s - interfaceName] = 0;

	if (*s == 0)
	{
		// This is the interface short name rather than the subspace
		auto idNewItem = tree.AppendItem(idNamespace);
		char srcDesc[256];
		SafeFormat(srcDesc, "interface: %-43s  %s", subspace, sInterfaceDef.Tree().Source().Name());
		tree.SetItemText(srcDesc, idNewItem);

		tree.SetItemImage(idNewItem, 2);

		for (int i = 2; i < sInterfaceDef.NumberOfElements(); ++i)
		{
			cr_sex sMethod = sInterfaceDef[i];
			if (sMethod.NumberOfElements() > 1 && IsAtomic(sMethod[0]))
			{
				cstr method = sMethod[0].String()->Buffer;
				if (Eq(method, "extends") && sMethod.NumberOfElements() > 1)
				{
					auto idExtends = tree.AppendItem(idNewItem);
					char methodDesc[256];
					SafeFormat(methodDesc, "extends interface %s", AlwaysGetAtomic(sMethod[1]));
					tree.SetItemText(methodDesc, idExtends);
					tree.SetItemImage(idExtends, 6);
				}
				else if (Eq(method, "attribute") && sMethod.NumberOfElements() > 1)
				{
					auto idAttr = tree.AppendItem(idNewItem);
					char methodDesc[256];
					SafeFormat(methodDesc, "attribute %s", AlwaysGetAtomic(sMethod[1]));
					tree.SetItemText(methodDesc, idAttr);
					tree.SetItemImage(idAttr, 7);
				}
				else
				{
					auto idNewMethod = tree.AppendItem(idNewItem);
					char methodDesc[256];
					SafeFormat(methodDesc, "method: %s", sMethod[0].String()->Buffer);
					tree.SetItemText(methodDesc, idNewMethod);
					tree.SetItemImage(idNewMethod, 3);

					bool isInput = true;

					for (int i = 1; i < sMethod.NumberOfElements(); ++i)
					{
						cr_sex sArg = sMethod[i];

						if (IsAtomic(sArg))
						{
							isInput = false;

							if (i + 1 < sMethod.NumberOfElements() && !IsAtomic(sMethod[i + 1]))
							{
								auto idMap = tree.AppendItem(idNewMethod);
								tree.SetItemText("->", idMap);
							}
							continue;
						}

						auto idArg = tree.AppendItem(idNewMethod);

						char argText[256];
						if (sArg.NumberOfElements() == 2)
						{
							cstr type = AlwaysGetAtomic(sArg[0]);
							cstr localVarName = AlwaysGetAtomic(sArg[1]);

							SafeFormat(argText, "(%s %s)", type, localVarName);

							tree.SetItemText(argText, idArg);
							tree.SetItemImage(idArg, isInput ? 9 : 10);
						}
						else
						{
							tree.SetItemText("<unknown arg>", idArg);
						}
					}
				}
			}
		}
	}
	else
	{
		ID_TREE_ITEM idSubspace = GetSubspace(tree, idNamespace, subspace);
		InsertInterface(tree, idSubspace, s + 1, sInterfaceDef);
	}
}

void TreeManager::InsertMacro(IGuiTree& tree, ID_TREE_ITEM idNamespace, cstr macroName, cr_sex sMacroDef)
{
	cstr s;
	for (s = macroName; *s != '.' && *s != 0; s++)
	{
	}

	char* subspace = (char*)_alloca(s - macroName + 1);
	memcpy(subspace, macroName, s - macroName);
	subspace[s - macroName] = 0;

	if (*s == 0)
	{
		if (sMacroDef.NumberOfElements() == 5)
		{
			cr_sex sDirective = sMacroDef[4];
			if (sDirective.NumberOfElements() == 2)
			{
				if (Eq(AlwaysGetAtomic(sDirective[0]), "out.AddAtomic"))
				{
					if (IsStringLiteral(sDirective[1]))
					{
						auto idNewItem = tree.AppendItem(idNamespace);
						char srcDesc[256];
						SafeFormat(srcDesc, "enum { %-28s = %s }", subspace, sDirective[1].String()->Buffer);
						char desc[256];
						SafeFormat(desc,  "%-55s %s", srcDesc, sMacroDef.Tree().Source().Name());
						tree.SetItemText(desc, idNewItem);
						tree.SetItemImage(idNewItem, 11);
						return;
					}
				}
				auto idNewItem = tree.AppendItem(idNamespace);
				char srcDesc[256];
				SafeFormat(srcDesc, "macro: %-32s  %s", subspace, sMacroDef.Tree().Source().Name());
				tree.SetItemText(srcDesc, idNewItem);
			}
		}
	}
	else
	{
		ID_TREE_ITEM idSubspace = GetSubspace(tree, idNamespace, subspace);
		InsertMacro(tree, idSubspace, s + 1, sMacroDef);
	}
}


void TreeManager::InsertStruct(IGuiTree& tree, ID_TREE_ITEM idTypes, cstr structName, cr_sex sStructDef)
{
	// This is the interface short name rather than the subspace
	auto idNewItem = tree.AppendItem(idTypes);

	char srcDesc[256];
	SafeFormat(srcDesc, "struct %-32s %s", structName, sStructDef.Tree().Source().Name());
	tree.SetItemText(srcDesc, idNewItem);
	tree.SetItemImage(idNewItem, 4);

	for (int i = 2; i < sStructDef.NumberOfElements(); ++i)
	{
		cr_sex sField = sStructDef[i];
		if (sField.NumberOfElements() > 1)
		{
			cstr type = AlwaysGetAtomic(sField[0]);
			cstr value = AlwaysGetAtomic(sField[1]);

			char fieldDesc[256];
			SafeFormat(fieldDesc, "%s %s", type, value);

			auto idField = tree.AppendItem(idNewItem);
			tree.SetItemText(fieldDesc, idField);
			tree.SetItemImage(idField, 5);

			for (int32 j = 2; j < sField.NumberOfElements(); j++)
			{
				cstr valueN = AlwaysGetAtomic(sField[j]);
				char fieldDescN[256];
				SafeFormat(fieldDescN, "%s %s", type, valueN);

				auto idFieldN = tree.AppendItem(idNewItem);
				tree.SetItemText(fieldDescN, idFieldN);
				tree.SetItemImage(idFieldN, 5);
			}
		}
	}
}

void TreeManager::InsertFunction(IGuiTree& tree, ID_TREE_ITEM idFunction, cstr fnName, cr_sex sFunctionDef)
{
	// This is the interface short name rather than the subspace
	auto idNewItem = tree.AppendItem(idFunction);

	char srcDesc[256];
	SafeFormat(srcDesc, "%s", sFunctionDef.Tree().Source().Name());

	char desc[256];
	SafeFormat(desc, "%-32s %s", fnName, srcDesc);

	tree.SetItemText(desc, idNewItem);
	tree.SetItemImage(idNewItem, 8);

	bool isInput = true;

	for (int i = 2; i < sFunctionDef.NumberOfElements(); ++i)
	{
		cr_sex sArg = sFunctionDef[i];

		if (IsAtomic(sArg))
		{
			if (Eq(sArg.String()->Buffer, ":"))
			{
				break;
			}
			else
			{
				isInput = false;

				if (i + 1 < sFunctionDef.NumberOfElements() && !IsAtomic(sFunctionDef[i+1]))
				{
					auto idMap = tree.AppendItem(idNewItem);
					tree.SetItemText("->", idMap);
				}
				continue;
			}
		}

		auto idArg = tree.AppendItem(idNewItem);

		char argText[256];
		if (sArg.NumberOfElements() == 2)
		{
			cstr type = AlwaysGetAtomic(sArg[0]);
			cstr localVarName = AlwaysGetAtomic(sArg[1]);

			SafeFormat(argText, "(%s %s)", type, localVarName);

			tree.SetItemText(argText, idArg);
			tree.SetItemImage(idArg, isInput ? 9 : 10);
		}
		else
		{
			tree.SetItemText("<unknown arg>", idArg);
		}
	}
}

void TreeManager::BuildNamespaces(IGuiTree& tree, ID_TREE_ITEM idNamespace, ID_TREE_ITEM idTypes, ID_TREE_ITEM idFunctions, cr_sex sRoot)
{
	for (int i = 0; i < sRoot.NumberOfElements(); ++i)
	{
		if (match_compound(sRoot[i], 2, keywordNamespace, ParseAtomic,
			[this, &tree, idNamespace](cr_sex s, const fstring& sKeyword, const fstring& nsText)
			{
				InsertNamespaceUnique(tree, idNamespace, nsText);
			}
		)) continue;
		
		enum { MAX_METHODS_AND_ATTRIBUTES_PER_INTERFACE = 32766};
		if (match_compound(sRoot[i], MAX_METHODS_AND_ATTRIBUTES_PER_INTERFACE, keywordInterface, ParseAtomic,
			[this, &tree, idNamespace](cr_sex s, const fstring& sKeyword, const fstring& nsText)
			{
				InsertInterface(tree, idNamespace, nsText, s);
			}
		)) continue;

		enum { MAX_FIELDS_PER_STRUCT = 32766 };
		if (match_compound(sRoot[i], MAX_FIELDS_PER_STRUCT + 2, keywordStruct, ParseAtomic,
			[this, &tree, idTypes](cr_sex s, const fstring& sKeyword, const fstring& structName)
			{
				InsertStruct(tree, idTypes, structName, s);
			}
		)) continue;

		enum { MAX_ARGS_PER_FUNCTION = 128 };
		if (match_compound(sRoot[i], MAX_ARGS_PER_FUNCTION + 2, keywordFunction, ParseAtomic,
			[this, &tree, idFunctions](cr_sex s, const fstring& sKeyword, const fstring& fnName)
			{
				InsertFunction(tree, idFunctions, fnName, s);
			}
		)) continue;

		enum { MAX_MACRO_LEN = 128};
		if (match_compound(sRoot[i], MAX_MACRO_LEN, keywordMacro, ParseAtomic,
			[this, &tree, idNamespace](cr_sex s, const fstring& sKeyword, const fstring& macroName)
			{
				InsertMacro(tree, idNamespace, macroName, s);
			}
		)) continue;

		enum { MAX_ALIAS_LEN = 3 };
		if (match_compound(sRoot[i], MAX_ALIAS_LEN, keywordAlias, ParseAtomic, ParseAtomic,
			[this, &tree, idNamespace](cr_sex s, const fstring& sKeyword, const fstring& aliasFrom, const fstring& aliasTo)
			{
				InsertAlias(tree, idNamespace, aliasFrom, aliasTo, s);
			}
		)) continue;
	}
}

int CompareFuncNodes(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	TreeManager& tm = *(TreeManager*)lParamSort;
	return tm.Compare(lParam1, lParam2);
}

class SexyExplorer: IObserver
{
private:
	WidgetContext wc;
	ISplitScreen& screen;
	IGuiTree* classTree;

	TreeManager treeManager;

	void SortRecursive(ID_TREE_ITEM idBranch)
	{
		HWND hTree = classTree->TreeWindow();

		TVSORTCB sorter;
		sorter.hParent = (HTREEITEM)idBranch;
		sorter.lParam = (LPARAM)&treeManager;
		sorter.lpfnCompare = CompareFuncNodes;

		TreeView_SortChildrenCB(hTree, &sorter, &treeManager);

		ID_TREE_ITEM idChild = (ID_TREE_ITEM) TreeView_GetChild(hTree, idBranch);
		while (idChild != 0)
		{
			TVITEMA item = { 0 };
			item.mask = TVIF_HANDLE | TVIF_PARAM;
			item.hItem = (HTREEITEM)idChild;
			TreeView_GetItem(hTree, &item);

			TreeManager::ItemDesc* desc = (TreeManager::ItemDesc*) item.lParam;
			if (desc && desc->type == TreeManager::ItemDesc::Type::NAMESPACE)
			{
				SortRecursive(idChild);
			}
			idChild = (ID_TREE_ITEM) TreeView_GetNextSibling(hTree, idChild);
		}
	}

	void OnMetaChanged(ISXYMetaTable& meta)
	{
		classTree->Clear();

		struct : IEventCallback<MetaInfo>
		{
			IGuiTree* classTree;
			ID_TREE_ITEM idNamespaces;
			ID_TREE_ITEM idTypes;
			ID_TREE_ITEM idFunction;
			TreeManager* treeManager;
			void OnEvent(MetaInfo& info) override
			{
				if (info.pRoot)
				{
					treeManager->BuildNamespaces(*classTree, idNamespaces, idTypes, idFunction, *info.pRoot);
				}
			}
		} buildNamespaces;
		buildNamespaces.idNamespaces = classTree->AppendItem(0);
		buildNamespaces.classTree = classTree;
		buildNamespaces.idTypes = classTree->AppendItem(0);
		buildNamespaces.idFunction = classTree->AppendItem(0);
		buildNamespaces.treeManager = &treeManager;
		classTree->SetItemText("Namespaces", buildNamespaces.idNamespaces);
		classTree->SetItemText("Types", buildNamespaces.idTypes);
		classTree->SetItemText("Functions", buildNamespaces.idFunction);

		classTree->SetItemImage(buildNamespaces.idNamespaces, 1);
		classTree->SetItemImage(buildNamespaces.idTypes, 4);
		classTree->SetItemImage(buildNamespaces.idFunction, 8);

		meta.ForEverySXYFile(buildNamespaces);

		SortRecursive(buildNamespaces.idNamespaces);
	}

	void OnEvent(Event& ev) override
	{
		if (ev == evMetaUpdated)
		{
			auto& metaEv = As<TEventArgs<ISXYMetaTable*>>(ev);
			OnMetaChanged(*metaEv.value);
		}
	}
public:
	SexyExplorer(WidgetContext _wc, ISplitScreen& _screen) : wc(_wc), screen(_screen)
	{
		screen.SetBackgroundColour(RGBAb(128, 128, 192));

		ITabSplitter* tabs = CreateTabSplitter(*screen.Children());
		tabs->SetVisible(true);

		auto& classView = tabs->AddTab();
		classView.SetName("Class View");

		TreeStyle style;
		style.hasButtons = true;
		style.hasCheckBoxes = false;
		style.hasLines = true;
		classTree = CreateTree(classView.Children(), style);
		classTree->SetImageList(13, IDB_BLANK, IDB_NAMESPACE, IDB_INTERFACE, IDB_METHOD, IDB_STRUCT, IDB_FIELD, IDB_EXTENDS, IDB_ATTRIBUTE, IDB_FUNCTION, IDB_INPUT, IDB_OUTPUT, IDB_ENUM, IDB_ALIAS);

		SendMessageA(classTree->TreeWindow(), WM_SETFONT, (WPARAM) (HFONT) _wc.fontSmallLabel, 0);

		Widgets::AnchorToParent(*classTree, 0, 0, 0, 0);

		classTree->SetVisible(true);

		wc.publisher.Subscribe(this, evMetaUpdated);
	}

	~SexyExplorer()
	{
		wc.publisher.Unsubscribe(this);
	}
};

void Main(IMessagePump& pump)
{
	AutoFree<IPublisherSupervisor> publisher(Rococo::Events::CreatePublisher());

	LOGFONTA lineEditorLF = { 0 };
	lineEditorLF.lfHeight = -12;
	lineEditorLF.lfCharSet = ANSI_CHARSET;
	lineEditorLF.lfOutPrecision = OUT_DEFAULT_PRECIS;
	lineEditorLF.lfClipPrecision = CLIP_DEFAULT_PRECIS;
	lineEditorLF.lfQuality = CLEARTYPE_QUALITY;
	SafeFormat(lineEditorLF.lfFaceName, "Consolas");

	Font smallCaptionFont(lineEditorLF);

	WidgetContext context{ *publisher, smallCaptionFont };
	AutoFree<ITheme> theme = UseNamedTheme("Classic", context.publisher);

	AutoFree<IIDEFrameSupervisor> ide = CreateMainIDEFrame(context);
	Widgets::SetText(*ide, "Sexy Studio");
	Widgets::SetSpan(*ide, 1024, 600);

	struct IDEEvents : IObserver
	{
		IMessagePump* pump = nullptr;
		IIDEFrameSupervisor* ide = nullptr;

		void OnEvent(Rococo::Events::Event& ev) override
		{
			if (ev == evIDEClose)
			{
				pump->Quit();
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
	} ideEvents;
	ideEvents.pump = &pump;
	ideEvents.ide = ide;
	
	auto* splitscreen = CreateSplitScreen(ide->Children());
	Widgets::AnchorToParent(*splitscreen, 0, 0, 0, 0);

	splitscreen->SetBackgroundColour(RGBAb(192, 128, 128));

	splitscreen->SplitIntoColumns(400);

	auto* projectView = splitscreen->GetFirstHalf();
	auto* sourceView = splitscreen->GetSecondHalf();

	PropertySheets propertySheets(*projectView, *ide);
	SexyExplorer explorer(context, *sourceView);

	publisher->Subscribe(&ideEvents, evIDEClose);
	publisher->Subscribe(&ideEvents, evIDEMax);
	publisher->Subscribe(&ideEvents, evIDEMin);

	ide->SetVisible(true);
	splitscreen->SetVisible(true);
	ide->LayoutChildren();

	propertySheets.CollapseTree();

	AutoFree<IStringBuilder> heapStringBuilder = CreateDynamicStringBuilder(1024);

	auto& sb = heapStringBuilder->Builder();

	Rococo::SexyStudio::AppendDescendantsAndRectsToString(*ide, sb);

	OutputDebugStringA(*sb);

	pump.MainLoop();
}


int APIENTRY WinMain(
	_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPSTR lpCmdLine,
	_In_ int nShowCmd)
{
	Rococo::OS::SetBreakPoints(Rococo::OS::BreakFlag_All & ~Rococo::OS::BreakFlag_IllFormed_SExpression);

	try
	{
		HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
		if FAILED(hr)
		{
			Throw(hr, "%s: CoInitializeEx failed", __FUNCTION__);
		}

		InitStudioWindows(hInstance, (LPCSTR)IDI_ICON1, (LPCSTR)IDI_ICON2);

		BufferedPaintInit();

		Win32MessagePump pump;
		Main(pump);

		BufferedPaintUnInit();
		return 0;
	}
	catch (IException& ex)
	{
		Rococo::OS::ShowErrorBox(Windows::NoParent(), ex, "Sexy Studio - Error!");
		return ex.ErrorCode();
	}
}


