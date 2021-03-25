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

using namespace Rococo;
using namespace Rococo::SexyStudio;
using namespace Rococo::Events;

auto evClose = "EvMainIDEWindowCloseRequest"_event;

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
	char contentFolder[128] = "C:\\work\\rococo\\content";
}

namespace Rococo::SexyStudio
{
	void PopulateTreeWithSXYFiles(IGuiTree& tree, cstr contentFolder);
}

void BuildPropertySheets(ISplitScreen& screen)
{
	screen.SetBackgroundColour(RGBAb(128, 192, 128));

	ITabSplitter* tabs = CreateTabSplitter(*screen.Children());
	tabs->SetVisible(true);

	ITab& propTab = tabs->AddTab();
	propTab.SetName("Properties");
	propTab.SetTooltip("Property View");

	ITab& projectTab = tabs->AddTab();
	projectTab.SetName("Projects");
	projectTab.SetTooltip("Project View");

	ITab& settingsTab = tabs->AddTab();
	settingsTab.SetName("Settings");
	settingsTab.SetTooltip("Global Configuration Settings");
	IVariableList* globalSettings = CreateVariableList(settingsTab.Children());
	globalSettings->SetVisible(true);

	IAsciiStringEditor* contentEditor = globalSettings->AddAsciiString();
	contentEditor->SetName("Content");
	contentEditor->Bind(Globals::contentFolder, sizeof Globals::contentFolder);
	contentEditor->SetVisible(true);

	TreeStyle style;
	style.hasButtons = true;
	style.hasLines = true;

	auto* tree = CreateTree(projectTab.Children(), style);
	Widgets::AnchorToParent(*tree, 0, 0, 0, 0);
	tree->SetVisible(true);

	PopulateTreeWithSXYFiles(*tree, Globals::contentFolder);
}

void BuildSourceSheets(ISplitScreen& screen)
{
	screen.SetBackgroundColour(RGBAb(128, 128, 192));

	ITabSplitter* tabs = CreateTabSplitter(*screen.Children());
	tabs->SetVisible(true);

	ITab& src1 = tabs->AddTab();
	src1.SetName("main.cpp");
	src1.SetTooltip("C:\\work\\main.cpp");

	ITab& src2 = tabs->AddTab();
	src2.SetName("aux.cpp");
	src2.SetTooltip("C:\\work\\aux.cpp");
}

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

	BuildPropertySheets(*projectView);
	BuildSourceSheets(*sourceView);

	publisher->Subscribe(&ideEvents, evIDEClose);
	publisher->Subscribe(&ideEvents, evIDEMax);
	publisher->Subscribe(&ideEvents, evIDEMin);

	ide->SetVisible(true);
	splitscreen->SetVisible(true);
	ide->LayoutChildren();

	pump.MainLoop();
}


int APIENTRY WinMain(
	_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPSTR lpCmdLine,
	_In_ int nShowCmd)
{
	Rococo::OS::SetBreakPoints(Rococo::OS::BreakFlag_All);

	InitStudioWindows(hInstance, (LPCSTR)IDI_ICON1, (LPCSTR)IDI_ICON2);

	BufferedPaintInit();

	try
	{
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


