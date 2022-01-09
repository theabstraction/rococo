
#define NOMINMAX
#include <windows.h>

#define IMPORT_SEXY_WINDOWS_LIB
#include <sexy.windows.h>

#include <rococo.libs.inl>

#include <rococo.window.h>

#include <rococo.win32.resources.h>

int  WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	using namespace Rococo;
	using namespace Rococo::SexyWindows;

	Rococo::SexyWindows::SexyWindowsSupervisor sws(hInstance);

	ErrorDialogSpec e;
	e.parent = SysWindowHandle::None();
	e.title = "Error Test!";
	e.systemError = ERROR_ACCESS_DENIED;

	const char* messages[] =  { "C:\\Program Files\\MyBad\\bad.exe", "Error opening file", nullptr };
	e.errorMessages.pArray = messages;

	const char* buttons[] = { "Retry", "Abort", nullptr };
	e.responseButtons.pArray = buttons;

//	auto result = sws().ShowErrorDialog(e);

	try
	{
		class TestException : public IException
		{
			cstr Message() const override
			{
				return "Naughy cat!";
			}

			int32 ErrorCode() const override
			{
				return ERROR_ACCOUNT_RESTRICTION;
			}

			Debugging::IStackFrameEnumerator* StackFrames()  override
			{
				return nullptr;
			}
		} ex;
		throw ex;
		Throw(ERROR_INVALID_ACCOUNT_NAME, "Test");
	}
	catch (IException& ex)
	{
		Rococo::Windows::ExceptionDialogSpec spec =
		{
			hInstance,
			{ 30, 200, 520, 220, 180 },
			(cstr) IDD_EXCEPTION_DIALOG,
			IDC_STACKVIEW,
			IDC_LOGVIEW,
			"sexy.windows.test Exception Dialog"
		};

		Rococo::Windows::ShowExceptionDialog(spec, nullptr, ex);
	}
	/*
	ScriptedDialogSpec sds;
	sds.parent = SysWindowHandle::None();
	sds.scriptFile = "refresh.sxy";
	sds.indicatorFile = "C:\\work\\rococo\\rococo.sexy.windows.test\\installation\\sexy.windows.test.marker";
	sds.maxProgSize = 1048576 ;
	sws().ShowScriptedDialog(sds);
*/
	return 0;
}