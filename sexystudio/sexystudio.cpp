// sexystudio.cpp : Defines the entry point for the application.
// Copyright (c) 2021 Mark Anthony Taylor

#include <rococo.libs.inl>
#include "sexystudio.impl.h"
#include "resource.h"

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

void Main(IMessagePump& pump)
{
	struct IDEEvents : IObserver
	{
		IMessagePump* pump = nullptr;

		void OnEvent(Rococo::Events::Event& ev) override
		{
			if (ev == evIDEClose)
			{
				pump->Quit();
			}
		}
	} ideEvents;
	ideEvents.pump = &pump;

	AutoFree<IPublisherSupervisor> publisher(Rococo::Events::CreatePublisher());
	publisher->Subscribe(&ideEvents, evIDEClose);

	AutoFree<IIDEFrameSupervisor> ide = CreateMainIDEFrame(*publisher);
	UseDefaultFrameBarLayout(ide->FrameBar());
	AddDefaultCloseButton(*publisher, ide->FrameBar(), evIDEClose);
	ide->SetCloseEvent(evIDEClose);
	ide->SetVisible(true);

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


