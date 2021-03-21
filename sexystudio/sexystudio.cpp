// sexystudio.cpp : Defines the entry point for the application.
// Copyright (c) 2021 Mark Anthony Taylor

#include <rococo.libs.inl>
#include "sexystudio.impl.h"
#include "resource.h"
#include "rococo.auto-release.h"

#include <rococo.events.h>

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

void BuildPropertySheets(IPublisher& publisher, ISplitScreen& screen)
{
	screen.SetVisible(true);

	AutoRelease<ITab> projectTab = CreateTab();
	projectTab->SetName("Projects");
	projectTab->SetTooltip("Project View");

	AutoRelease<ITab> propertyTab = CreateTab();
	propertyTab->SetName("Properties");
	propertyTab->SetTooltip("Property View");

	ITabSplitter* tabs = CreateTabSplitter(publisher, *screen.Children());
	tabs->SetVisible(true);

	TabDefiniton propertyDef{ propertyTab };
	tabs->AddTab(propertyDef);

	TabDefiniton projectDef{ projectTab };
	tabs->AddTab(projectDef);
}

void BuildSourceSheets(IPublisher& publisher, ISplitScreen& screen)
{
	screen.SetVisible(true);

	AutoRelease<ITab> srcMainTab = CreateTab();
	srcMainTab->SetName("main.cpp");
	srcMainTab->SetTooltip("C:\\work\\main.cpp");

	AutoRelease<ITab> srcAuxTab = CreateTab();
	srcAuxTab->SetName("aux.cpp");
	srcAuxTab->SetTooltip("C:\\work\\aux.cpp");

	ITabSplitter* tabs = CreateTabSplitter(publisher, *screen.Children());
	tabs->SetVisible(true);;

	TabDefiniton src1Def{ srcMainTab };
	tabs->AddTab(src1Def);

	TabDefiniton src2Def{ srcAuxTab };
	tabs->AddTab(src2Def);
}

void Main(IMessagePump& pump)
{
	AutoFree<IPublisherSupervisor> publisher(Rococo::Events::CreatePublisher());

	AutoFree<IIDEFrameSupervisor> ide = CreateMainIDEFrame(*publisher);
	Widgets::SetText(ide->Window(), "Sexy Studio");

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

	auto* splitscreen = CreateSplitScreen(*publisher, ide->Children());
	Widgets::AnchorToParent(*splitscreen, 0, 0, 0, 0);

	ide->Children().Add(splitscreen);

	splitscreen->SetVisible(true);

	splitscreen->SplitIntoColumns(400);
	splitscreen->SetBackgroundColour(RGBAb(192, 128, 128));

	auto* projectView = splitscreen->GetFirstHalf();
	projectView->SetBackgroundColour(RGBAb(128, 192, 128));
	auto* sourceView = splitscreen->GetSecondHalf();
	sourceView->SetBackgroundColour(RGBAb(128, 128, 192));

	BuildPropertySheets(*publisher, *projectView);
	BuildSourceSheets(*publisher, *sourceView);

	publisher->Subscribe(&ideEvents, evIDEClose);
	publisher->Subscribe(&ideEvents, evIDEMax);
	publisher->Subscribe(&ideEvents, evIDEMin);

	ide->SetVisible(true);
	splitscreen->Layout();

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

	try
	{
		Win32MessagePump pump;
		Main(pump);
		return 0;
	}
	catch (IException& ex)
	{
		Rococo::OS::ShowErrorBox(Windows::NoParent(), ex, "Sexy Studio - Error!");
		return ex.ErrorCode();
	}
}


