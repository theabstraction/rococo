#ifdef _WIN32
#include <rococo.os.win32.h>
#include <wincon.h>
#endif

#include <rococo.os.h>
#include <rococo.sexy.ide.h>

#define ROCOCO_USE_SAFE_V_FORMAT
#include <rococo.strings.h>

#include <rococo.visitors.h>
#include <rococo.ide.h>

using namespace Rococo;

#ifdef _WIN32
struct ConsoleColourController : Strings::IColourOutputControl
{
	HANDLE hConsole;

	ConsoleColourController(): hConsole(GetStdHandle(STD_OUTPUT_HANDLE))
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

	void AddDisassembly(RGBAb, cstr, RGBAb, bool) override
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

	void SetCodeHilight(cstr, const Vec2i&, const Vec2i&, cstr) override
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
	IDebuggerWindow* GetConsoleAsDebuggerWindow(Strings::IVarArgStringFormatter& formatter, Strings::IColourOutputControl& control)
	{
		return new ConsoleAsDebuggerWindow(formatter, control);
	}

	Strings::IColourOutputControl& GetConsoleColourController()
	{
		static ConsoleColourController controller;
		return controller;
	}

	Strings::IVarArgStringFormatter& GetStdoutFormatter()
	{
		static StdoutFormatter formatter;
		return formatter;
	}
}