#include <rococo.types.h>
#include <rococo.os.win32.h>
#include <rococo.window.h>
#include "resource.h"

#include <Rococo.api.h>
#include <rococo.os.h>

#define ROCOCO_USE_SAFE_V_FORMAT
#include <rococo.strings.h>

#ifdef _DEBUG
#pragma comment(lib, "rococo.windows.debug.lib")
#pragma comment(lib, "rococo.util.debug.lib")
#pragma comment(lib, "rococo.audio.debug.lib")
#else
#pragma comment(lib, "rococo.windows.lib")
#pragma comment(lib, "rococo.util.lib")
#pragma comment(lib, "rococo.audio.lib")
#endif

namespace
{
	using namespace Rococo;

	struct TestException : public IException
	{
		char msg[256];
		int32 errorCode;

		virtual cstr Message() const
		{
			return msg;
		}

		virtual int32 ErrorCode() const
		{
			return errorCode;
		}

		Debugging::IStackFrameEnumerator* StackFrames() override { return nullptr; }
	};
}

namespace
{
	using namespace Rococo;
	using namespace Rococo::Windows;

	enum { IDSHOWMODAL = 1001 };

	class MainWindowHandler : public StandardWindowHandler
	{
	private:
		IDialogSupervisor* window;

		MainWindowHandler() : window(nullptr)
		{

		}

		~MainWindowHandler()
		{
			Rococo::Free(window); // top level windows created with CreateDialogWindow have to be manually freed
		}

		void PostConstruct()
		{
			WindowConfig config;
			SetOverlappedWindowConfig(config, Vec2i{ 800, 600 }, SW_SHOWMAXIMIZED, nullptr, "Rococo Audio Test App", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 0);

			window = Windows::CreateDialogWindow(config, this); // Specify 'this' as our window handler
			AddPushButton(*window, GuiRect(10, 10, 200, 34), "Close", IDCANCEL, 0); // Child window is auto freed when the parent is deleted
		}
	public:
		// This is our post construct pattern. Allow the constructor to return to initialize the v-tables, then call PostConstruct to create the window 
		static MainWindowHandler* Create()
		{
			auto m = new MainWindowHandler();
			m->PostConstruct();
			return m;
		}

		void Free()
		{
			delete this;
		}

		virtual void OnMenuCommand(HWND hWnd, DWORD id)
		{
			if (id == IDCANCEL)
			{
				PostQuitMessage(0);
			}
		}

		virtual void OnClose(HWND hWnd)
		{
			PostQuitMessage(0);
		}
	};
}

#include <rococo.io.h>
#include <rococo.audio.h>
#include <objbase.h>

void Test(Rococo::Audio::IAudio& audio)
{

}

int CALLBACK WinMain(HINSTANCE _hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	using namespace Rococo;
	using namespace Rococo::Windows;

	try
	{
		HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
		if FAILED(hr)
		{
			Throw(hr, "CoInitializeEx failed");
		}

		AutoFree<IAllocatorSupervisor> audioHeap(Memory::CreateBlockAllocator(16384, 0));
		Rococo::Audio::SetAudioAllocator(audioHeap);

		AutoFree<IOSSupervisor> os = GetOS();
		AutoFree<IInstallationSupervisor> installation = CreateInstallation(L"content.indicator.txt", *os);

		Audio::AudioConfig audio_config{};
		AutoFree<Audio::IOSAudioAPISupervisor> osAudio = Audio::CreateOSAudio();
		AutoFree<Audio::IAudioSupervisor> audio = Audio::CreateAudioSupervisor(*installation, *osAudio, audio_config);

		InitRococoWindows(_hInstance, LoadIcon(_hInstance, MAKEINTRESOURCE(IDI_ICON1)), LoadIcon(_hInstance, MAKEINTRESOURCE(IDI_ICON1)), nullptr, nullptr); // This must be called once, in WinMain or DllMain
		AutoFree<MainWindowHandler> mainWindowHandler(MainWindowHandler::Create());

		auto jungleMP3 = audio->Bind3DSample("!sounds/jungle.mp3"_fstring);
		if (!jungleMP3)
		{
			Throw(0, "Failed to bind jungle.mp3 to a sample index");
		}

		Rococo::Audio::AudioSource3D source = { 0 };
		source.priority = 1;
		source.volume = 1.0f;

		auto id = audio->Play3DSound(jungleMP3, source, 0);
		if (!id)
		{
			Throw(0, "Failed to play a 3D sound");
		}

		bool isRunning = true;

		OS::ticks start = OS::CpuTicks();
		OS::ticks period = OS::CpuHz() / 10;

		while (isRunning)
		{
			MsgWaitForMultipleObjectsEx(0, NULL, 50, QS_ALLINPUT, 0);

			MSG msg;
			while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
			{
				if (msg.message == WM_QUIT)
				{
					isRunning = false;
					break;
				}
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}

			OS::ticks end = OS::CpuTicks();

			if (end - start > period)
			{
				start = end;

				id = audio->Play3DSound(jungleMP3, source, 0);
			}

			audio->ThrowOnThreadError();
		}
	}
	catch (IException& ex)
	{
		Rococo::Windows::ShowErrorBox(NoParent(), ex, "Test threw an exception");
	}

}