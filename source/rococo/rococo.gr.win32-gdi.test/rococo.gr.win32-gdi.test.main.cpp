#include <rococo.types.h>
#include <rococo.os.h>
#include <rococo.io.h>
#include <rococo.strings.h>
#include <rococo.gr.win32-gdi.h>
#include <rococo.gui.retained.ex.h>
#include <rococo.sexml.h>
#include <rococo.functional.h>

#pragma comment (lib,"Gdiplus.lib")

using namespace Rococo;
using namespace Rococo::Gui;

IGR2DScene* TestScene();
void TestWidgets(IGRClientWindow& client);

void RunMessageLoop(IGRClientWindow& client)
{
	client.QueuePaint();

	struct EventHandler : IGREventHandler
	{
		EGREventRouting OnGREvent(GRWidgetEvent& ev) override
		{
			if (ev.eventType == EGRWidgetEventType::EDITOR_UPDATED)
			{
				auto& editorEv = static_cast<GRWidgetEvent_EditorUpdated&>(ev);
				return OnEditorEvent(editorEv);
			}
			else
			{
				return EGREventRouting::Terminate;
			}
		}

		EGREventRouting OnEditorEvent(GRWidgetEvent_EditorUpdated& ev)
		{
			UNUSED(ev);
			return EGREventRouting::Terminate;
		}
	} evHandler;

	client.SetEventHandler(&evHandler);

	MSG msg;
	while (GetMessage(&msg, nullptr, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);

		client.GRSystem().DispatchMessages();
	}
}

void TestGreatSex(IGRClientWindow& client, cstr sexmlFilePath);
void TestPropertyEditor(IGRClientWindow& client);

using namespace Rococo::Sex::SEXML;

int MainProtected(cstr commandLine)
{
	GR::Win32::GRMainFrameConfig config;
	AutoFree<GR::Win32::IGRMainFrameWindowSupervisor> mainFrame = GR::Win32::CreateGRMainFrameWindow(NULL, config);
	auto& client = mainFrame->Client();
	client.BindStandardXBOXControlsToVKeys();
	client.GRSystem().SetFocusOverlayRenderer(&GetDefaultFocusRenderer());
	client.LinkScene(TestScene());

	auto initClient = [&client](const ISEXMLDirectiveList& items)
		{
			size_t startIndex = 0;
			if (FindDirective(items, "PropertyEditor", REF startIndex))
			{
				TestPropertyEditor(client);
			}
			else
			{
				startIndex = 0;
				auto* gs = FindDirective(items, "GreatSex", REF startIndex);
				if (gs)
				{
					auto& aSexml = (*gs)["Sexml"];
					cstr sexmlFile = AsString(aSexml).c_str();
					TestGreatSex(client, sexmlFile);
				}
				else
				{
					Throw(0, "Expecting either PropertyEditor or GreatSex directive");
				}
			}
		};

	if (*commandLine != 0)
	{
		OS::ParseSXMLFromString("Command-Line", commandLine, initClient);
	}
	else
	{
		cstr section = "rococo.gr.gdi.test";
		if (OS::IsUserSEXMLExistant(nullptr, section))
		{
			OS::LoadUserSEXML(nullptr, "rococo.gr.gdi.test", initClient);
		}
		else
		{
			U8FilePath sexmlPath;
			OS::GetUserSEXMLFullPath(OUT sexmlPath, nullptr, section);
			Throw(0, "No command line arguments given and %s does not exist.\r\n Example:\r\n\t(GreatSex (Sexml !tests/greatsex.test.sexml))", sexmlPath.buf);
		}
	}

	return 0;
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR commandLine, int nShowCmd)
{
	UNUSED(nShowCmd);
	UNUSED(hInstance);
	UNUSED(hPrevInstance);
	
	try
	{
		Rococo::OS::SetBreakPoints(OS::Flags::BreakFlag_All);
		AutoFree<Rococo::GR::Win32::IWin32GDIApp> gdiApp = Rococo::GR::Win32::CreateWin32GDIApp();
		int result = MainProtected(commandLine);
		return result;
	}
	catch (IException& ex)
	{
		Rococo::Windows::ShowErrorBox(Rococo::Windows::NoParent(), ex, "GR Win32 GDI Test error");
		return ex.ErrorCode();
	}
}
