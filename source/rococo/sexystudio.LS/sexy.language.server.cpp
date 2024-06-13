#include <rococo.types.h>
#include <rococo.os.win32.h>
#include <stdio.h>
#include <rococo.os.h>

using namespace Rococo;

bool PumpAvailableMessages()
{
	MSG msg;
	while (PeekMessage(& msg, NULL, NULL, NULL, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);

		if (msg.message == WM_QUIT || msg.message == WM_CLOSE)
		{
			return false;
		}
	}

	return true;
}

struct Reader
{
	char data[1025];
	OVERLAPPED overlapped = { 0 };
	HANDLE hStdIn;
	HANDLE hLog;

	Reader(): hStdIn(GetStdHandle(STD_INPUT_HANDLE))
	{
		cstr logFile = "C:\\work\\rococo\\SexyStudioBin\\ls.log";
		hLog = CreateFileA(logFile, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hLog == INVALID_HANDLE_VALUE)
		{
			Throw(GetLastError(), "Error creating log %s: ", logFile);
		}
	}

	~Reader()
	{
		CloseHandle(hLog);
	}

	void OnReadStdin(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered)
	{
		if (dwErrorCode != 0)
		{
			Throw(dwErrorCode, "Error ReadFileEx(hStdIn, ...)");
		}

		if (dwNumberOfBytesTransfered < sizeof data)
		{
			data[dwNumberOfBytesTransfered] = 0;
			OnRead(dwNumberOfBytesTransfered);
		}

		Prep();
	}

	static void StaticOnReadStdin(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped)
	{
		Reader* reader = (Reader*)lpOverlapped->hEvent;
		reader->OnReadStdin(dwErrorCode, dwNumberOfBytesTransfered);
	}

	NOT_INLINE void OnRead(DWORD nChars)
	{
		DWORD bytesWritten = 0;
		WriteFile(hLog, data, (DWORD) nChars + 1, &bytesWritten, NULL);
	}

	void Prep()
	{
		overlapped.hEvent = (HANDLE) this;

		if (!ReadFileEx(hStdIn, data, sizeof data, &overlapped, StaticOnReadStdin))
		{
			HRESULT err = HRESULT_FROM_WIN32(GetLastError());
			Throw(err, "Error ReadFileEx(hStdIn, ...)");
		}
	}
};

int mainProtected(int argc, char* argv[])
{
	UNUSED(argc);
	UNUSED(argv);

	Reader reader;
	reader.Prep();

	for (;;)
	{
		MsgWaitForMultipleObjectsEx(0, NULL, 1000, QS_ALLINPUT, MWMO_ALERTABLE);

		if (!PumpAvailableMessages())
		{
			break;
		}
	}

	return 0;
}

int main(int argc, char* argv[])
{
	try
	{
		return mainProtected(argc, argv);
	}
	catch (IException& ex)
	{
		char err[256];
		OS::FormatErrorMessage(err, sizeof err, ex.ErrorCode());
		fprintf(stderr, "%s: Exception thrown: %s code %d\n%s\n", argv[0], ex.Message(), ex.ErrorCode(), err);
		return ex.ErrorCode();
	}
}