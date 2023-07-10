#include <rococo.types.h>
#include <rococo.os.win32.h>
#include <rococo.window.h>
#include "resource.h"

#include <Rococo.api.h>
#include <rococo.os.h>

#define ROCOCO_USE_SAFE_V_FORMAT
#include <rococo.strings.h>

#include <rococo.io.h>
#include <rococo.audio.h>
#include <objbase.h>

#include <rococo.time.h>

#ifdef _DEBUG
#pragma comment(lib, "rococo.windows.debug.lib")
#pragma comment(lib, "rococo.util.debug.lib")
#pragma comment(lib, "rococo.audio.debug.lib")
#else
#pragma comment(lib, "rococo.windows.lib")
#pragma comment(lib, "rococo.util.lib")
#pragma comment(lib, "rococo.audio.lib")
#endif

using namespace Rococo;
using namespace Rococo::IO;

struct PositionButton
{
	cstr name;
	Vec3 soundPosition;
	GuiRect buttonRect;
};

PositionButton compassButtons[] =
{
	{ "NW", {-1,  1, 0}, {  50,  60, 100, 80 } },
	{ "N",  { 0,  1, 0}, { 120,  60, 170, 80 } },
	{ "NE", { 1,  1, 0}, { 190,  60, 240, 80 } },
	{ "W",  {-1,  0, 0}, {  50,  90, 100,110 } },
	{ "O",  { 0,  0, 0}, { 120,  90, 170,110 } },
	{ "E",  { 1,  0, 0}, { 190,  90, 240,110 } },
	{ "SW", {-1, -1, 0}, {  50, 120, 100,140 } },
	{ "S",  { 0, -1, 0}, { 120, 120, 170,140 } },
	{ "SE", { 1, -1, 0}, { 190, 120, 240,140 } },
};


struct ElevationButton
{
	cstr name;
	float height;
	GuiRect buttonRect;
};

ElevationButton elevationButtons[] =
{
	{ "High",  2, {  50,  260, 100, 300 } },
	{ "Eye",   0, { 120,  260, 170, 300 } },
	{ "Low",  -2, { 190,  260, 240, 300 } },
};

struct SampleButton
{
	cstr name;
	GuiRect buttonRect;
	cstr pingPath;
};

SampleButton sampleButtons[] =
{
	{ "Cat",  {  50,  310, 100, 350 }, "!sounds/cat.mp3" },
	{ "Dog",  { 120,  310, 170, 350 }, "!sounds/dog.mp3" },
	{ "Owl",  { 190,  310, 240, 350 }, "!sounds/owl.mp3" },
};

namespace
{
	using namespace Rococo;
	using namespace Rococo::Strings;
	using namespace Rococo::Windows;

	enum { IDSHOWMODAL = 1001 };

	class MainWindowHandler : public StandardWindowHandler
	{
	private:
		Rococo::Audio::IAudio& audio;
		IDialogSupervisor* window;

		enum class Ids: ControlId {COMPASS_ID_START = 8000, STATUS_BAR = 8500, ELEVATION_ID_START = 9000, SAMPLE_ID_START = 9500};

		char statusBuffer[256];
		Vec3 soundPosition{ 0,0,0 };
		char bearing[3] = "O";
		float height = 0;
		char elevation[10] = "eye";
		char err[128] = "";
		Audio::IdSample sampleId;

		MainWindowHandler(Audio::IAudio& _audio) : audio(_audio), window(nullptr)
		{
			
		}

		~MainWindowHandler()
		{
			Rococo::Free(window); // top level windows created with CreateDialogWindow have to be manually freed
		}

		void PostConstruct()
		{
			WindowConfig config;
			SetOverlappedWindowConfig(config, Vec2i{ 800, 600 }, SW_SHOWNORMAL, nullptr, "Rococo Audio Test App", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 0);

			window = Windows::CreateDialogWindow(config, this); // Specify 'this' as our window handler

			RECT clientRect;
			GetClientRect(*window, &clientRect);

			ControlId nCompassDirections = sizeof compassButtons / sizeof PositionButton;
			for (ControlId i = 0; i < nCompassDirections; i++)
			{
				AddPushButton(*window, compassButtons[i].buttonRect, compassButtons[i].name, ((ControlId) Ids::COMPASS_ID_START) + i, 0);
			}

			size_t nElevations = sizeof elevationButtons / sizeof ElevationButton;
			for (ControlId i = 0; i < nElevations; i++)
			{
				AddPushButton(*window, elevationButtons[i].buttonRect, elevationButtons[i].name, ((ControlId)Ids::ELEVATION_ID_START) + i, 0);
			}

			ControlId nSamples = sizeof sampleButtons / sizeof SampleButton;
			for (ControlId i = 0; i < nSamples; i++)
			{
				AddPushButton(*window, sampleButtons[i].buttonRect, sampleButtons[i].name, ((ControlId) Ids::SAMPLE_ID_START) + i, 0);
			}

			GuiRect labelRect;
			labelRect.left = clientRect.left + 10;
			labelRect.top = clientRect.bottom - 40;
			labelRect.right = clientRect.right - 10;
			labelRect.bottom = clientRect.bottom - 10;
			Rococo::Windows::AddLabel(*window, labelRect, "", (ControlId) Ids::STATUS_BAR, WS_BORDER);
			FormatStatus();
		}
	public:
		// This is our post construct pattern. Allow the constructor to return to initialize the v-tables, then call PostConstruct to create the window 
		static MainWindowHandler* Create(Rococo::Audio::IAudio& audio)
		{
			auto m = new MainWindowHandler(audio);
			m->PostConstruct();
			return m;
		}

		void Free()
		{
			delete this;
		}

		void OnGetMinMaxInfo(HWND, MINMAXINFO& info) override
		{
			info.ptMinTrackSize = { 800,600 };
			info.ptMaxTrackSize = { 800,600 };
		}

		void FormatStatus()
		{
			if (*err)
			{
				SafeFormat(statusBuffer, "%s + %s (%.2f, %.2f, %.2f). Error: %s", bearing, elevation, soundPosition.x, soundPosition.y, soundPosition.z, err);
			}
			else
			{
				SafeFormat(statusBuffer, "%s + %s (%.2f, %.2f, %.2f)", bearing, elevation, soundPosition.x, soundPosition.y, soundPosition.z);
			}
			SetDlgItemTextA(*window, (ControlId)Ids::STATUS_BAR, statusBuffer);
		}

		void OnMenuCommand(HWND, DWORD id) override
		{
			ControlId nCompassDirections = sizeof compassButtons / sizeof PositionButton;
			ControlId nElevations = sizeof elevationButtons / sizeof ElevationButton;
			ControlId nSamples = sizeof sampleButtons / sizeof SampleButton;

			if (id == IDCANCEL)
			{
				PostQuitMessage(0);
			}
			else if (id >= (ControlId) Ids::COMPASS_ID_START && id <= (ControlId)Ids::COMPASS_ID_START + nCompassDirections)
			{
				ControlId index = id - (ControlId)Ids::COMPASS_ID_START;
				auto& button = compassButtons[index];
				soundPosition = button.soundPosition;
				soundPosition.z = height;
				Rococo::Strings::CopyString(bearing, sizeof bearing, button.name);
			}
			else if (id >= (ControlId)Ids::ELEVATION_ID_START && id <= (ControlId)Ids::ELEVATION_ID_START + nElevations)
			{
				ControlId index = id - (ControlId)Ids::ELEVATION_ID_START;
				auto& button = elevationButtons[index];
				height = soundPosition.z = button.height;
				Rococo::Strings::CopyString(elevation, sizeof elevation, button.name);
			}
			else if (id >= (ControlId)Ids::SAMPLE_ID_START && id <= (ControlId)Ids::SAMPLE_ID_START + nSamples)
			{
				ControlId index = id - (ControlId)Ids::SAMPLE_ID_START;
				auto& button = sampleButtons[index];
				sampleId = audio.Bind3DSample(to_fstring(button.pingPath));
				if (!sampleId)
				{
					SafeFormat(err, sizeof err, "Failed to bind %s", button.pingPath);
				}
				else
				{
					Audio::AudioSource3D source;
					source.dopplerVelocity = { 0,0,0 };
					source.position = soundPosition;
					source.priority = 1;
					source.volume = 1.0f;
					source.msDelay = 0;					
					audio.Play3DSound(sampleId, source, 0);
					*err = 0;
				}
			}

			FormatStatus();
		}

		void OnClose(HWND) override
		{
			PostQuitMessage(0);
		}

		void OnTick(Time::ticks /* start */, Time::ticks /* frameStart */, Time::ticks /* dt */, Time::ticks /* tickHz */)
		{

		}
	};
}

int CALLBACK WinMain(HINSTANCE _hInstance, HINSTANCE /* hPrevInstance */, LPSTR /* lpCmdLine */, int /* nCmdShow */)
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

		AutoFree<IOSSupervisor> ios = GetIOS();
		AutoFree<IInstallationSupervisor> installation = CreateInstallation(L"content.indicator.txt", *ios);

		AutoFree<Audio::IAudioInstallationSupervisor> audioInstallation = Audio::CreateAudioInstallation(*installation);

		Audio::AudioConfig audio_config{};
		AutoFree<Audio::IOSAudioAPISupervisor> osAudio = Audio::CreateOSAudio();
		AutoFree<Audio::IAudioSupervisor> audio = Audio::CreateAudioSupervisor(*audioInstallation, *osAudio, audio_config);

		InitRococoWindows(_hInstance, LoadIcon(_hInstance, MAKEINTRESOURCE(IDI_ICON1)), LoadIcon(_hInstance, MAKEINTRESOURCE(IDI_ICON1)), nullptr, nullptr); // This must be called once, in WinMain or DllMain
		AutoFree<MainWindowHandler> mainWindowHandler(MainWindowHandler::Create(*audio));

		bool isRunning = true;

		Time::ticks frameStart = Time::TickCount();
		Time::ticks start = frameStart;

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

				mainWindowHandler->OnTick(start, frameStart, Time::TickCount() - frameStart, Time::TickHz());
				frameStart = Time::TickCount();

				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}

			audio->ThrowOnThreadError();
		}
	}
	catch (IException& ex)
	{
		Rococo::Windows::ShowErrorBox(NoParent(), ex, "Test threw an exception");
	}

}