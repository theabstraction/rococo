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
	U8FilePath contentFolder{ "\\work\\rococo\\content\\scripts\\" };
	U8FilePath packageFolder{ "\\work\\rococo\\content\\packages\\mhost_1000.sxyz" };
	U8FilePath searchPath{ "MHost/" };
}

namespace Rococo::SexyStudio
{
	void PopulateTreeWithSXYFiles(IGuiTree& tree, cstr contentFolder, ISexyDatabase& database, IIDEFrame& frame);
}

auto evMetaUpdated = "sexystudio.meta.updated"_event;

class PropertySheets: IObserver, IGuiTreeRenderer
{
private:
	WidgetContext wc;
	IIDEFrame& ideFrame;
	IGuiTree* fileBrowser = nullptr;
	AutoFree<ISexyDatabaseSupervisor> database;
	ITab* projectTab = nullptr;

	void OnEvent(Event& ev) override
	{
		if (ev == evContentChange)
		{
			WaitCursorSection waitSection;
			ideFrame.SetProgress(0.0f, "Populating file browser...");
			PopulateTreeWithSXYFiles(*fileBrowser, Globals::contentFolder, *database, ideFrame);
		//	PopulateTreeWithPackages(Globals::searchPath, Globals::packageFolder, *database);
			ideFrame.SetProgress(100.0f, "Populated file browser");

			TEventArgs<ISexyDatabase*> args;
			args.value = database;
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
		database(CreateSexyDatabase())
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
		Widgets::ExpandBottomFromTop(*projectSettings, 32);

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

	ISexyDatabase& Database() { return *database;  }

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

	void AppendFunctions(ISxyNamespace& ns, ID_TREE_ITEM idNSNode, ISexyDatabase& database, bool appendSourceName)
	{
		for (int i = 0; i < ns.FunctionCount(); ++i)
		{
			auto& function = ns.GetFunction(i);
			auto idFunction = classTree->AppendItem(idNSNode);
			cstr publicName = function.PublicName();
			auto* localFunction = function.LocalFunction();

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
		for (int i = 0; i < ns.Length(); ++i)
		{
			auto& subspace = ns[i];
			auto idBranch = classTree->AppendItem(idNSNode);
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

		AppendNamespaceRecursive(database.GetRootNamespace(), idNamespace, database);
	}

	void OnEvent(Event& ev) override
	{
		if (ev == evMetaUpdated)
		{
			auto& evDB = As<TEventArgs<ISexyDatabase*>>(ev);
			OnMetaChanged(*evDB.value);
		}
	}
public:
	SexyExplorer(WidgetContext _wc, ISplitScreen& _screen) : wc(_wc), screen(_screen)
	{
		screen.SetBackgroundColour(RGBAb(128, 128, 192));

		ITabSplitter* tabs = CreateTabSplitter(*screen.Children());
		tabs->SetVisible(true);

		classTab = &tabs->AddTab();
		classTab->SetName("Class View");

		TreeStyle style;
		style.hasButtons = true;
		style.hasCheckBoxes = false;
		style.hasLines = true;
		classTree = CreateTree(classTab->Children(), style);
		classTree->SetImageList(15, IDB_BLANK, IDB_NAMESPACE, IDB_INTERFACE, IDB_METHOD, IDB_STRUCT, IDB_FIELD, IDB_EXTENDS, IDB_ATTRIBUTE, IDB_FUNCTION, IDB_INPUT, IDB_OUTPUT, IDB_ENUM, IDB_ALIAS, IDB_FACTORY, IDB_ARCHETYPE);

		SendMessageA(classTree->TreeWindow(), WM_SETFONT, (WPARAM) (HFONT) _wc.fontSmallLabel, 0);

		Widgets::AnchorToParent(*classTree, 0, 0, 0, 0);

		classTree->SetVisible(true);

		wc.publisher.Subscribe(this, evMetaUpdated);
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

	propertySheets.SelectProjectTab();
	explorer.SelectClassTreeTab();

	TEventArgs<bool> nullArgs;
	publisher->Publish(nullArgs, evContentChange);

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


