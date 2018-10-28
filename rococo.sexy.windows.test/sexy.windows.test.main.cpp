#include <windows.h>

#define IMPORT_SEXY_WINDOWS_LIB
#include <sexy.windows.h>

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

//	sws().ShowExceptionDialog(SysWindowHandle::None());

	ScriptedDialogSpec sds;
	sds.parent = SysWindowHandle::None();
	sds.scriptFile = "refresh.sxy";
	sds.indicatorFile = "C:\\work\\rococo\\rococo.sexy.windows.test\\installation\\sexy.windows.test.marker";
	sds.maxProgSize = 1048576 ;
	sws().ShowScriptedDialog(sds);

	return 0;
}