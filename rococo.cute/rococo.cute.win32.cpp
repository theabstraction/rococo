#include <rococo.api.h>
#include <rococo.win32.resources.h>
#include <rococo.os.win32.h>
#include <rococo.window.h>
#include <rococo.libs.inl>
#include <rococo.io.h>
#include <rococo.strings.h>
#include <rococo.cute.h>

#include <sexy.lib.script.h>

#include <sexy.types.h>
#include <sexy.vm.h>
#include <sexy.vm.cpu.h>
#include <sexy.script.h>

#include <rococo.post.h>

using namespace Rococo;
using namespace Rococo::Cute;

int Main(IInstallation& installation, IMasterWindowFactory& factory);

int RunMessageLoop(IMasterWindowFactory& factory)
{
	MSG msg;
	while (factory.HasInstances() && GetMessageA(&msg, nullptr, 0, 0))
	{
		if (msg.message == WM_QUIT)
		{
			return (int) msg.wParam;
		}
		TranslateMessage(&msg);
		DispatchMessageA(&msg);
	}
	return 0;
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	try
	{
		Rococo::Windows::InitRococoWindows(hInstance, nullptr, nullptr, nullptr, nullptr);
		AutoFree<Rococo::IOSSupervisor> os(GetOS());
		AutoFree<IInstallationSupervisor> installation(CreateInstallation(L"cute.test.marker", *os));

		wchar_t srcpath[Rococo::IO::MAX_PATHLEN];
		SecureFormat(srcpath, sizeof(srcpath), L"%sscripts\\native\\", installation->Content());
		Rococo::Script::SetDefaultNativeSourcePath(srcpath);

	//	Rococo::OS::SetBreakPoints(Rococo::OS::BreakFlag_All);

		AutoFree<Post::IPostboxSupervisor> postbox ( Post::CreatePostbox() );
		
		AutoFree<IMasterWindowFactory> factory = CreateMasterWindowFactory(hInstance, nullptr, *postbox);

		return Main(*installation, *factory);
	}
	catch (IException& ex)
	{
		HANDLE hRichEditor = LoadLibraryA("Riched20.dll");
		Rococo::Windows::ExceptionDialogSpec spec =
		{
			hInstance,
			{
				30,
				300,
				600,
				180,
				200
			},
			(const char*)IDD_EXCEPTION_DIALOG,
			IDC_STACKVIEW,
			IDC_LOGVIEW,
			"Rococo.Cute Test Exception Dialog"
		};
		Rococo::Windows::ShowExceptionDialog(spec, nullptr, ex);
		return ex.ErrorCode();
	}
}
