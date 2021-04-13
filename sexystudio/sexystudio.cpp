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

ID_TREE_ITEM GetSubspace(IGuiTree& tree, ID_TREE_ITEM idCurrentNS, cstr subspace)
{
	/*
	ID_TREE_ITEM idBranch = 0;

	auto findMatchingSubspacesLambda = [&tree, &idBranch, idCurrentNS, subspace](TreeItemInfo& info)
	{
		char branch[256];
		tree.GetText(branch, sizeof branch, info.idItem);

		if (Eq(branch, subspace))
		{
			idBranch = info.idItem;
		}
	};

	struct ANON1 : IEventCallback<TreeItemInfo>
	{
		void OnLambdaEvent(TreeItemInfo& info)
		{

		}

		void OnEvent(TreeItemInfo& info) override
		{
			OnLambdaEvent(info);
		}
	}

	*/

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
		return idNewItem;
	}
	else
	{
		return findMatchingSubspace.idBranch;
	}
}

void InsertNamespaceUnique(IGuiTree& tree, ID_TREE_ITEM idNamespace, cstr namespaceText)
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

void BuildNamespaces(IGuiTree& tree, ID_TREE_ITEM idNamespace, cr_sex s)
{
	for (int i = 0; i < s.NumberOfElements(); ++i)
	{
		auto& sTopLevelItem = s[i];
		if (sTopLevelItem.NumberOfElements() == 2)
		{
			auto& sKey = sTopLevelItem[0];
			auto& sValue = sTopLevelItem[1];

			if (IsAtomic(sKey) && IsAtomic(sValue))
			{
				auto key = sKey.String()->Buffer;
				auto value = sValue.String()->Buffer;
				if (Eq(key, "namespace"))
				{
					InsertNamespaceUnique(tree, idNamespace, value);
				}
			}
		}
	}
}

class SexyExplorer: IObserver
{
private:
	WidgetContext wc;
	ISplitScreen& screen;
	IGuiTree* classTree;

	void OnMetaChanged(ISXYMetaTable& meta)
	{
		classTree->Clear();

		struct : IEventCallback<MetaInfo>
		{
			IGuiTree* classTree;
			ID_TREE_ITEM idNamespaces;
			void OnEvent(MetaInfo& info) override
			{
				if (info.pRoot)
				{
					BuildNamespaces(*classTree, idNamespaces, *info.pRoot);
				}
			}
		} buildNamespaces;
		buildNamespaces.idNamespaces = classTree->AppendItem(0);
		buildNamespaces.classTree = classTree;
		classTree->SetItemText("Namespaces", buildNamespaces.idNamespaces);

		meta.ForEverySXYFile(buildNamespaces);
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


