#include "..\rococo.mplat\mplat.monitors.cpp"

using namespace Rococo::IO;

#include <stdio.h>
#include <windows.h>
#include <synchapi.h>
#include <consoleapi.h>
#include <WinCon.h>

static int row = 0;

int main(int argc, char* argv[])
{
	struct ANON: IO::IShaderMonitorEvents
	{
		void OnLog(IO::IShaderMonitor&, IO::EShaderLogPriority p, cstr file, cstr text) override
		{
			switch (p)
			{
			case IO::EShaderLogPriority::Cosmetic:
				printf("%s%s", file, text);
				break;
			case IO::EShaderLogPriority::ErrorCode:
				printf("%s", text);
				break;
			case IO::EShaderLogPriority::Error:
				printf("%s: %s", file, text);
				break;
			default:
				{
					row++;
					U8FilePath path;
					int nChars = Format(path, "%.128s", file);

					//if ((row % 2) == 0)
					{
						if (nChars > 0 && nChars < 125)
						{
							for (int i = nChars; i < 128; i++)
							{
								path.buf[i] = '.';
							}
						}

						path.buf[128] = 0;
					}

					printf("%-128s %s", path.buf, text);
				}					
				break;
			}
		}

		void OnModifiedFileSkipped(IO::IShaderMonitor&, cstr hlslFilename) override
		{
			UNUSED(hlslFilename);
		}
	} logger;


	bool isInteractive = true;

	for (int i = 0; i < argc; ++i)
	{
		cstr switchString = argv[i];
		if (Eq(switchString, "-?"))
		{
			printf("Usage: %s. Optional switches are:\n\t-? to show this help and\n\t-A for automatic mode (Q does not quit)\n", argv[0]);
			return 0;
		}
		if (Eq(switchString, "-A"))
		{
			isInteractive = false;
		}
	}

	AutoFree<IShaderMonitor> monitor = TryCreateShaderMonitor(nullptr, logger);
	if (!monitor)
	{
		fprintf(stderr, "Could not create shader monitor.");
		return -1;
	}

	monitor->CompileDirectory(nullptr);

	printf("Identified %llu shaders that need compiling...\n", monitor->QueueLength());

	HANDLE hConsoleIn = GetStdHandle(STD_INPUT_HANDLE);

	DWORD oldMode;
	GetConsoleMode(hConsoleIn, &oldMode);
	// disable mouse and window input
	BOOL mewMode = oldMode & ~(ENABLE_MOUSE_INPUT | ENABLE_WINDOW_INPUT);
	SetConsoleMode(hConsoleIn, mewMode);

	FlushConsoleInputBuffer(hConsoleIn);

	while (monitor->QueueLength() != 0)
	{
		monitor->DoHousekeeping();

		if (isInteractive)
		{
			DWORD eventCount;
			while (GetNumberOfConsoleInputEvents(hConsoleIn, &eventCount) && eventCount > 0)
			{
				INPUT_RECORD record[1];
				DWORD eventsRead;
				if (ReadConsoleInputA(hConsoleIn, record, 1, &eventsRead) && eventsRead == 1)
				{
					if (record->EventType == KEY_EVENT)
					{
						if (!record->Event.KeyEvent.bKeyDown && tolower(record->Event.KeyEvent.uChar.AsciiChar) == 'q')
						{
							goto end;
						}
					}
				}
			}
		}

		WaitForSingleObject(hConsoleIn, 250);
	}

	monitor->DoHousekeeping();
end:
	SetConsoleMode(hConsoleIn, oldMode);

	return 0;
}