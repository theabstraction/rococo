#include <rococo.types.h>
#include <rococo.os.h>
#include <rococo.io.h>
#include <rococo.strings.h>
#include <rococo.os.win32.h>
#include <rococo.gr.win32-gdi.h>
#include <rococo.gui.retained.ex.h>

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

void TestGreatSex(IGRClientWindow& client);
void TestPropertyEditor(IGRClientWindow& client);

struct FocusRenderer : Gui::IGRSystemSubRenderer
{
	void Render(IGRPanel& panel, IGRRenderContext& g, const GuiRect& clipRect) override
	{
		g.EnableScissors(clipRect);
		g.DrawRect(panel.AbsRect(), RGBAb(255,255,0, 32), panel.RectStyle(), panel.CornerRadius());
		g.DisableScissors();
	}
} s_FocusRenderer;

int MainProtected()
{
	GR::Win32::GRMainFrameConfig config;
	AutoFree<GR::Win32::IGRMainFrameWindowSupervisor> mainFrame = GR::Win32::CreateGRMainFrameWindow(NULL, config);
	auto& client = mainFrame->Client();
	client.BindStandardXBOXControlsToVKeys();
	client.GRSystem().SetFocusOverlayRenderer(&s_FocusRenderer);
	client.LinkScene(TestScene());
	// TestGreatSex(client);
	TestPropertyEditor(client);
	// TestWidgets(client);

	return 0;
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR commandLine, int nShowCmd)
{
	UNUSED(nShowCmd);
	UNUSED(hInstance);
	UNUSED(hPrevInstance);
	UNUSED(commandLine);

	try
	{
		Rococo::OS::SetBreakPoints(OS::Flags::BreakFlag_All);
		AutoFree<Rococo::GR::Win32::IWin32GDIApp> gdiApp = Rococo::GR::Win32::CreateWin32GDIApp();
		int result = MainProtected();
		return result;
	}
	catch (IException& ex)
	{
		Rococo::Windows::ShowErrorBox(Rococo::Windows::NoParent(), ex, "GR Win32 GDI Test error");
		return ex.ErrorCode();
	}
}
