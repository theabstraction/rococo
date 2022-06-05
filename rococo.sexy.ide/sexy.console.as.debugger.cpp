#include <rococo.sexy.ide.h>

#include <sexy.types.h>
#include <sexy.compiler.public.h>
#include <sexy.debug.types.h>
#include <sexy.script.h>
#include <Sexy.S-Parser.h>

#include <windows.h>
#include <rococo.window.h>

#define ROCOCO_USE_SAFE_V_FORMAT
#include <rococo.strings.h>

#include <rococo.visitors.h>
#include <commctrl.h>

#include <vector>
#include <algorithm>
#include <unordered_map>

#include <rococo.io.h>
#include <rococo.ide.h>
#include <rococo.os.h>

#include <wincon.h>

using namespace Rococo;

struct ConsoleAsDebuggerWindow: public Rococo::IDebuggerWindow
{
	void AddDisassembly(RGBAb colour, cstr text, RGBAb bkColor = RGBAb(255, 255, 255), bool bringToView = false) override
	{

	}

	void InitDisassembly(size_t codeId) override
	{

	}

	void AddSourceCode(cstr name, cstr sourceCode) override
	{

	}

	void Free() override
	{
		delete this;
	}

	[[nodiscard]] Windows::IWindow& GetDebuggerWindowControl() override
	{
		return Windows::NoParent();
	}

	void PopulateMemberView(Visitors::ITreePopulator& populator) override
	{

	}

	void PopulateRegisterView(Visitors::IListPopulator& populator) override
	{

	}

	void PopulateVariableView(Visitors::IListPopulator& populator) override
	{

	}

	void PopulateCallStackView(Visitors::IListPopulator& populator) override
	{

	}

	void Run(IDebuggerPopulator& populator, IDebugControl& control) override
	{

	}

	void SetCodeHilight(cstr source, const Vec2i& start, const Vec2i& end, cstr message) override
	{

	}

	void ShowWindow(bool show, IDebugControl* debugControl) override
	{

	}

	// Logger methods

	void AddLogSection(RGBAb colour, cstr format, ...) override
	{
		HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

		WORD attributes = 0;

		if (colour.red >= 64)
		{
			attributes |= FOREGROUND_RED;
		}

		if (colour.green >= 64)
		{
			attributes |= FOREGROUND_GREEN;
		}

		if (colour.blue >= 64)
		{
			attributes |= FOREGROUND_BLUE;
		}

		if (colour.red >= 128 || colour.green >= 128 || colour.blue >= 128)
		{
			attributes |= FOREGROUND_INTENSITY;
		}

		SetConsoleTextAttribute(hConsole, attributes);

		va_list args;
		va_start(args, format);
		vprintf(format, args);
		va_end(args);

		SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED);
	}

	void ClearLog()
	{
		
	}

	int Log(cstr format, ...)
	{
		HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

		SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_GREEN);

		va_list args;
		va_start(args, format);
		int length = vprintf(format, args);
		va_end(args);
		
		printf("\n");

		SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED);
		
		return length;
	}
};

namespace Rococo::Windows::IDE
{
	IDebuggerWindow* GetConsoleAsDebuggerWindow()
	{
		return new ConsoleAsDebuggerWindow();
	}
}