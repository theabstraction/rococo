#ifdef _WIN32
#include <rococo.os.win32.h>

namespace MSWindows
{
	extern "C" __declspec(dllimport) HANDLE GetStdHandle(DWORD nStdHandle);
	extern "C" __declspec(dllimport) BOOL SetConsoleTextAttribute(HANDLE hConsoleOutput, WORD wAttributes);

	enum Handles
	{
		STD_INPUT_HANDLE = -10,
		STD_OUTPUT_HANDLE = -11,
		STD_ERROR_HANDLE = -12
	};


	enum Colours
	{
		FOREGROUND_BLUE = 0x0001, // text color contains blue. 
		FOREGROUND_GREEN = 0x0002, // text color contains green.
		FOREGROUND_RED = 0x0004, // text color contains red.
		FOREGROUND_INTENSITY = 0x0008, // text color is intensified.
		BACKGROUND_BLUE = 0x0010, // background color contains blue.
		BACKGROUND_GREEN = 0x0020, // background color contains green.
		BACKGROUND_RED = 0x0040, // background color contains red.
		BACKGROUND_INTENSITY = 0x0080 // background color is intensified.
	};
}

#endif

#include <rococo.os.h>
#include <rococo.sexy.ide.h>


#include <rococo.strings.h>

#include <rococo.visitors.h>
#include <rococo.ide.h>

using namespace Rococo;

#ifdef _WIN32

using namespace MSWindows;

struct ConsoleColourController : Strings::IColourOutputControl
{
	HANDLE hConsole;

	ConsoleColourController(): hConsole(GetStdHandle((DWORD) STD_OUTPUT_HANDLE))
	{
	}

	void SetOutputColour(RGBAb colour) override
	{
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
	}
};
#else // Other OS's require specific implementation
struct ConsoleColourController : Strings::IColourOutputControl
{
	ConsoleColourController()
	{
	}

	void SetOutputColour(RGBAb colour) override
	{
	}
};
#endif

#include <stdio.h>

struct StdoutFormatter: Strings::IVarArgStringFormatter
{
	int PrintFV(const char* format, va_list args) override
	{
		return vprintf_s(format, args);
	}
};

struct ConsoleAsDebuggerWindow: public Rococo::IDebuggerWindow
{
	Strings::IVarArgStringFormatter& formatter;
	Strings::IColourOutputControl& colourControl;

	ConsoleAsDebuggerWindow(Strings::IVarArgStringFormatter& refFormatter, Strings::IColourOutputControl& refColourControl): formatter(refFormatter), colourControl(refColourControl)
	{

	}

	int PrintF(cstr format, ...)
	{
		va_list args;
		va_start(args, format);

		return formatter.PrintFV(format, args);
	}

	void AddDisassembly(DISASSEMBLY_TEXT_TYPE, cstr, bool) override
	{

	}

	void InitDisassembly(size_t) override
	{

	}

	void AddSourceCode(cstr, cstr) override
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

	void PopulateMemberView(Visitors::ITreePopulator&) override
	{

	}

	void PopulateRegisterView(Visitors::IListPopulator&) override
	{

	}

	void PopulateVariableView(Visitors::IListPopulator&) override
	{

	}

	void PopulateCallStackView(Visitors::IListPopulator& populator) override
	{
		struct ANON: Rococo::Visitors::IUIList
		{
			void AddRow(cstr values[]) override
			{
				for (cstr* p = values; *p != nullptr; p++)
				{
					cstr item = *p;
					console.PrintF("%s\t", item);
				}

				console.PrintF("\n");
			}

			void ClearRows() override
			{

			}

			void SetColumns(cstr columnNames[], int widths[]) override
			{
				UNUSED(widths);
				int nChars = 0;
				for (cstr* p = columnNames; *p != nullptr; p++)
				{
					cstr item = *p;
					nChars += console.PrintF("%s\t", item);
				}

				console.PrintF("\n");

				for (int i = 0; i < nChars; ++i)
				{
					console.PrintF("-");
				}

				console.PrintF("\n");
			}

			int NumberOfRows() const override
			{
				return 100;
			}

			void DeleteRow(int) override
			{

			}

			ConsoleAsDebuggerWindow& console;

			ANON(ConsoleAsDebuggerWindow& refConsole): console(refConsole)
			{

			}
		} consolePopulator(*this);
		populator.Populate(consolePopulator);
	}

	void Run(IDebuggerPopulator&, IDebugControl&) override
	{

	}

	void SetCodeHilight(cstr, const Vec2i&, const Vec2i&, cstr, bool) override
	{

	}

	void ShowWindow(bool, IDebugControl*) override
	{
		
	}

	// Logger methods

	void AddLogSection(RGBAb colour, cstr format, ...) override
	{
		colourControl.SetOutputColour(colour);

		va_list args;
		va_start(args, format);
		formatter.PrintFV(format, args);
		va_end(args);

		colourControl.SetOutputColour(RGBAb(128,128,128));
	}

	void ClearLog()override
	{
		
	}

	void ClearSourceCode() override
	{

	}

	void ResetJitStatus() override
	{

	}

	int Log(cstr format, ...) override
	{
		colourControl.SetOutputColour(RGBAb(0,128,128));

		va_list args;
		va_start(args, format);
		int length = formatter.PrintFV(format, args);
		va_end(args);
		
		PrintF("\n");

		colourControl.SetOutputColour(RGBAb(128, 128, 128));
		
		return length;
	}
};

namespace Rococo::Windows::IDE
{
	SEXYIDE_API IDebuggerWindow* GetConsoleAsDebuggerWindow(Strings::IVarArgStringFormatter& formatter, Strings::IColourOutputControl& control)
	{
		return new ConsoleAsDebuggerWindow(formatter, control);
	}

	SEXYIDE_API Strings::IColourOutputControl& GetConsoleColourController()
	{
		static ConsoleColourController controller;
		return controller;
	}

	SEXYIDE_API Strings::IVarArgStringFormatter& GetStdoutFormatter()
	{
		static StdoutFormatter formatter;
		return formatter;
	}
}