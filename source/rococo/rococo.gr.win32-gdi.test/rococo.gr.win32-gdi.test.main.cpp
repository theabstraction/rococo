#include <rococo.types.h>
#include <rococo.os.h>
#include <rococo.io.h>
#include <rococo.strings.h>
#include <rococo.os.win32.h>
#include <rococo.gr.win32-gdi.h>
#include <rococo.gui.retained.h>

#pragma comment (lib,"Gdiplus.lib")

using namespace Rococo;
using namespace Rococo::Gui;

IGR2DScene* TestScene();
void TestWidgets(IGRSystem& gr);

int MainProtected(HINSTANCE hInstance, cstr commandLine)
{
	AutoFree<GR::Win32::IGRMainFrameWindowSupervisor> mainFrame = GR::Win32::CreateGRMainFrameWindow(NULL);
	auto& client = mainFrame->Client();

	client.LinkScene(TestScene());

	TestWidgets(client.GRSystem());

	client.QueuePaint();

	MSG msg;
	while (GetMessage(&msg, nullptr, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 0;
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR commandLine, int nShowCmd)
{
	UNUSED(nShowCmd);
	UNUSED(hPrevInstance);

	try
	{
		Rococo::OS::SetBreakPoints(OS::Flags::BreakFlag_All);
		AutoFree<Rococo::GR::Win32::IWin32GDIApp> gdiApp = Rococo::GR::Win32::CreateWin32GDIApp();
		int result = MainProtected(hInstance, commandLine);
		return result;
	}
	catch (IException& ex)
	{
		Rococo::Windows::ShowErrorBox(Rococo::Windows::NoParent(), ex, "GR Win32 GDI Test error");
		return ex.ErrorCode();
	}
}
