#include <rococo.mplat.h>
#include <rococo.os.win32.h>
#include <rococo.window.h>
#include <rococo.sexy.ide.h>
#include <rococo.dx11.renderer.win32.h>
#define ROCOCO_USE_SAFE_V_FORMAT
#include <rococo.strings.h>
#include <rococo.imaging.h>
#include <rococo.strings.h>
#include <vector>
#include <algorithm>
#include <rococo.fonts.h>
#include <mplat.to.app.events.inl>
#include <sexy.script.h>
#include "mplat.panel.base.h"
#include <rococo.ide.h>
#include <mplat.audio.h>
#include <objbase.h>
#include <rococo.stl.allocators.h>
#include "mplat.components.h"

//////////////////////// XAUDIO2 and Media Foundation stuff for audio decoding ////////////////////
#pragma comment(lib, "wmcodecdspuuid.lib")
#pragma comment(lib, "Mfuuid.lib")
#pragma comment(lib, "Mfplat.lib")

#ifdef _DEBUG
# pragma comment(lib, "rococo.util.ex.debug.lib")
#else
# pragma comment(lib, "rococo.util.ex.lib")
#endif
///////////////////////////////////////////////////////////////////////////////////////////////////

#undef DrawText

#pragma warning (disable : 4250)

using namespace Rococo;
using namespace Rococo::Events;
using namespace Rococo::Windows;
using namespace Rococo::MPlatImpl;

static auto evFileUpdated = "OnFileUpdated"_event;

void PerformSanityTests();

namespace Rococo 
{
	namespace MPlatImpl
	{
		IPaneContainer* CreatePaneContainer(Platform& platform);
		IMathsVenue& GetOSVenue();
		IGuiStackSupervisor* CreateGui(IPublisher& publisher, ISourceCache& cache, IRenderer& renderer, IUtilitiies& utils);
	}
}

namespace Rococo
{
	namespace MPlatImpl
	{
		bool QueryYesNo(IWindow& ownerWindow, cstr message)
		{
			char title[256];
			GetWindowTextA(ownerWindow, title, 256);
			return ShowMessageBox(Windows::NullParent(), message, title, MB_ICONQUESTION | MB_YESNO) == IDYES;
		}

		void InitScriptSystem(IInstallation& installation);
	}
}

namespace Rococo
{
	namespace Graphics
	{
		IMeshBuilderSupervisor* CreateMeshBuilder(IRenderer& renderer);
	}
}

struct PlatformTabs: IObserver, IUIElement, public IMathsVenue
{
	Platform& platform;
	IMathsVenue* venue = nullptr;

	PlatformTabs(Platform& _platform):
		platform(_platform)
	{
		platform.publisher.Subscribe(this, evUIPopulate);
	}

	~PlatformTabs()
	{
		platform.publisher.Unsubscribe(this);
	}


	virtual void ShowVenue(IMathsVisitor& visitor)
	{
		platform.renderer.ShowWindowVenue(visitor);
	}

	void OnEvent(Event& ev) override
	{
		auto& pop = As<UIPopulate>(ev);

		venue = nullptr;

		if (Eq(pop.name, "overlay.window"))
		{
			pop.renderElement = this;
			venue = this;
		}
		else if (Eq(pop.name, "overlay.renderer"))
		{
			pop.renderElement = this;
			venue = platform.renderer.Venue();
		}
		else if (Eq(pop.name, "overlay.camera"))
		{
			pop.renderElement = this;
			venue = &platform.camera.Venue();
		}
		else if (Eq(pop.name, "overlay.textures"))
		{
			pop.renderElement = this;
			venue = platform.renderer.TextureVenue();
		}
		else if (Eq(pop.name, "overlay.meshes"))
		{
			pop.renderElement = this;
			venue = platform.meshes.Venue();
		}
		else if (Eq(pop.name, "overlay.os"))
		{
			pop.renderElement = this;
			venue = &GetOSVenue();
		}
		else if (Eq(pop.name, "overlay.cache"))
		{
			pop.renderElement = this;
			venue = platform.sourceCache.Venue();
		}
		else if (Eq(pop.name, "overlay.performance"))
		{
			pop.renderElement = this;
			venue = platform.utilities.Venue();
		}
		else
		{
		}
	}

	bool OnKeyboardEvent(const KeyboardEvent& key)  override
	{
		return platform.mathsVisitor.AppendKeyboardEvent(key);
	}

	void OnRawMouseEvent(const MouseEvent& me) override
	{
		platform.mathsVisitor.AppendMouseEvent(me);
	}

	void OnMouseMove(Vec2i cursorPos, Vec2i delta, int dWheel)  override
	{

	}

	void OnMouseLClick(Vec2i cursorPos, bool clickedDown) override
	{
		if (!clickedDown) platform.mathsVisitor.SelectAtPos(cursorPos);
	}

	void OnMouseRClick(Vec2i cursorPos, bool clickedDown)  override
	{

	}

	void Render(IGuiRenderContext& rc, const GuiRect& absRect)  override
	{
		platform.mathsVisitor.Clear();
		if (venue) venue->ShowVenue(platform.mathsVisitor);
		platform.mathsVisitor.Render(rc, absRect, 4);
	}

};

HINSTANCE g_Instance = nullptr;
cstr g_largeIcon = nullptr;
cstr g_smallIcon = nullptr;

ROCOCOAPI IMainloop
{
	virtual void Invoke(Platform& platform, HANDLE hInstanceLock, IDX11GraphicsWindow& mainWindow) = 0;
};

struct HandleManager
{
	HANDLE& hEvent;

	HandleManager(HANDLE& _hEvent): hEvent(_hEvent)
	{

	}

	~HandleManager()
	{
		if (hEvent != INVALID_HANDLE_VALUE)
		{
			CloseHandle(hEvent);
		}
	}
};

using namespace Rococo::Windows::IDE;

namespace Rococo
{
	namespace MPlatImpl
	{
		void InitArrayFonts(Platform& platform);
	}
}

int Main(HINSTANCE hInstance, IMainloop& mainloop, cstr title, HICON hLargeIcon, HICON hSmallIcon)
{
	using namespace Rococo::Components;

	Rococo::OS::SetBreakPoints(Rococo::OS::BreakFlag_All);

	U8FilePath exeFile;
	GetModuleFileNameA(nullptr, exeFile.buf, exeFile.CAPACITY);

	U8FilePath eventName = exeFile;
	for (char* p = eventName.buf; *p != 0; p++)
	{
		if (*p == '\\') *p = '#';
	}

	HANDLE hInstanceLock = CreateEventA(nullptr, TRUE, FALSE, eventName);

	int err = GetLastError();
	if (err == ERROR_ALREADY_EXISTS)
	{
		SetEvent(hInstanceLock);

		if (IsDebuggerPresent())
		{
			ShowMessageBox(Windows::NoParent(), "Application is already running", exeFile, MB_ICONEXCLAMATION);
		}

		return err;
	}

	HandleManager autoInstanceLock(hInstanceLock);

	LOGFONTA font = { 0 };
	SafeFormat(font.lfFaceName, "Consolas");
	font.lfHeight = 24;

	InitRococoWindows(hInstance, hLargeIcon, hSmallIcon, &font, &font);

	AutoFree<IAllocatorSupervisor> imageAllocator = Memory::CreateBlockAllocator(0, 0);
	Imaging::SetJpegAllocator(imageAllocator);
	Imaging::SetTiffAllocator(imageAllocator);

	AutoFree<IOSSupervisor> os = GetOS();
	AutoFree<IInstallationSupervisor> installation = CreateInstallation(L"content.indicator.txt", *os);
	AutoFree<IConfigSupervisor> config = CreateConfig();

	AutoFree<Rococo::Events::IPublisherSupervisor> publisher(Events::CreatePublisher());
	os->Monitor(installation->Content());

	AutoFree<IDX11Logger> logger = CreateStandardOutputLogger();

	FactorySpec factorySpec;
	factorySpec.hResourceInstance = hInstance;
	factorySpec.largeIcon = hLargeIcon;
	factorySpec.smallIcon = hSmallIcon;
	AutoFree<IDX11Factory> factory = CreateDX11Factory(*installation, *logger, factorySpec);

	WindowSpec ws;
	ws.exStyle = 0;
	ws.style = WS_OVERLAPPEDWINDOW;
	ws.hInstance = hInstance;
	ws.hParentWnd = nullptr;
	ws.messageSink = nullptr;
	ws.minSpan = { 1024, 640 };
	ws.X = CW_USEDEFAULT;
	ws.Y = CW_USEDEFAULT;
	ws.Width = 1152;
	ws.Height = 700;

	AutoFree<IDX11GraphicsWindow> mainWindow = factory->CreateDX11Window(ws, true);
	mainWindow->MakeRenderTarget();

	SetWindowTextA(mainWindow->Window(), title);

	AutoFree<ISourceCache> sourceCache(CreateSourceCache(*installation));
	AutoFree<IDebuggerEventHandler> debuggerEventHandler(CreateDebuggerEventHandler(*installation, mainWindow->Window()));
	AutoFree<OS::IAppControlSupervisor> appControl(OS::CreateAppControl());
	AutoFree<IDebuggerWindow> debuggerWindow(CreateDebuggerWindow(mainWindow->Window(), debuggerEventHandler->GetMenuCallback(), *appControl));

	Rococo::Memory::SetSexyAllocator(&sourceCache->Allocator());
	Rococo::MPlatImpl::InitScriptSystem(*installation);

	AutoFree<Rococo::Script::IScriptSystemFactory> ssFactory = CreateScriptSystemFactory_1_5_0_0();

	RunMPlatConfigScript(*config, *ssFactory, *debuggerWindow, *sourceCache, *appControl, nullptr);

	int32 maxEntities = config->GetInt("mplat.instances.entities.max"_fstring);
	if (maxEntities <= 0) Throw(0, "Int32 \"mplat.instances.entities.max\" defined in '!scripts/config_mplat.sxy' was not positive");

	OutputDebugStringA("\n\n");

	Audio::AudioConfig audio_config{};
	AutoFree<Audio::IAudioSupervisor> audio = Audio::CreateAudioSupervisor(*installation, audio_config);
	AutoFree<Rococo::Entities::IRigs> rigs = Rococo::Entities::CreateRigBuilder();
	AutoFree<Graphics::IMeshBuilderSupervisor> meshes = Graphics::CreateMeshBuilder(mainWindow->Renderer());

	AutoFree<IComponentFactory<IBodyComponent>> bodyFactory = CreateBodyFactory();
	AutoFree<IComponentFactory<ISkeletonComponent>> skeletonFactory = CreateSkeletonFactory();
	AutoFree<IComponentFactory<IParticleSystemComponent>> particleSystemFactory = CreateParticleSystemFactory();
	AutoFree<IComponentFactory<IRigsComponent>> rigsFactory = CreateRigsFactory();

	ComponentFactories factories
	{
		*bodyFactory,
		*skeletonFactory,
		*particleSystemFactory,
		*rigsFactory
	};

	AutoFree<IRCObjectTableSupervisor> ecs = Factories::Create_RCO_EntityComponentSystem(factories);

	AutoFree<Entities::IInstancesSupervisor> instances = Entities::CreateInstanceBuilder(*meshes, mainWindow->Renderer(), *publisher, *ecs, (size_t) maxEntities);
	AutoFree<Entities::IMobilesSupervisor> mobiles = Entities::CreateMobilesSupervisor(*instances);
	AutoFree<Graphics::ICameraSupervisor> camera = Graphics::CreateCamera(*instances, *mobiles, mainWindow->Renderer());
	AutoFree<Graphics::ISceneSupervisor> scene = Graphics::CreateScene(*instances, *camera, *rigs);
	AutoFree<IKeyboardSupervisor> keyboard = CreateKeyboardSupervisor();
	AutoFree<Graphics::ISpriteBuilderSupervisor> spriteBuilder = Graphics::CreateSpriteBuilderSupervisor(mainWindow->Renderer());
	AutoFree<Graphics::IRimTesselatorSupervisor> rimTesselator = Graphics::CreateRimTesselator();
	AutoFree<Graphics::IRodTesselatorSupervisor> rodTesselator = Graphics::CreateRodTesselator(*meshes);
	AutoFree<Entities::IParticleSystemSupervisor> particles = Entities::CreateParticleSystem(mainWindow->Renderer(), *instances);
	AutoFree<Graphics::IRendererConfigSupervisor> rendererConfig = Graphics::CreateRendererConfig(mainWindow->Renderer());
	AutoFree<IUtilitiesSupervisor> utilities = CreateUtilities(*installation, mainWindow->Renderer());
	AutoFree<IMathsVisitorSupervisor> mathsVisitor = CreateMathsVisitor(*utilities, *publisher);
	AutoFree<IGuiStackSupervisor> gui = CreateGui(*publisher, *sourceCache, mainWindow->Renderer(), *utilities);
	AutoFree<Graphics::IMessagingSupervisor> messaging = Graphics::CreateMessaging();

	Tesselators tesselators{ *rimTesselator, *rodTesselator };

	AutoFree<Joysticks::IJoystick_XBOX360_Supervisor> xbox360stick = Joysticks::CreateJoystick_XBox360Proxy();

	AutoFree<IInstallationManagerSupervisor> ims = Rococo::MPlatImpl::CreateIMS(*installation);

	AutoFree<IArchiveSupervisor> archive = Rococo::CreateArchive();

	AutoFree<IWorldSupervisor> world = Rococo::CreateWorld(*meshes, *instances);

	AutoFree<Graphics::ISpritesSupervisor> sprites = Rococo::Graphics::CreateSpriteTable(mainWindow->Renderer());
	
	Platform platform
	{ 
		*os, *installation, *appControl, mainWindow->Renderer(), mainWindow->Window(),* sprites,* rendererConfig,* messaging,
		*sourceCache, *debuggerWindow, *publisher, *utilities, *gui, *keyboard, *config, *archive, *meshes,
		*instances, *mobiles, *particles, *rigs, *spriteBuilder, *camera, *scene, tesselators, *mathsVisitor,
		*audio, *ssFactory, title, *xbox360stick, *ims, *world, *ecs
	};

	gui->PostConstruct(&platform);
	utilities->SetPlatform(platform);
	messaging->PostCreate(platform);

	PlatformTabs tabs(platform);

	mainloop.Invoke(platform, hInstanceLock, *mainWindow);

	return 0;
}

int Main(HINSTANCE hInstance, IAppFactory& appFactory, cstr title, HICON hLargeIcon, HICON hSmallIcon)
{
	CoInitializeEx(NULL, COINIT_MULTITHREADED);

	PerformSanityTests();

	struct : public IMainloop
	{
		IAppFactory* appFactory;

		void Invoke(Platform& platform, HANDLE hInstanceLock, IDX11GraphicsWindow& mainWindow) override
		{
			AutoFree<IApp> app(appFactory->CreateApp(platform));

			app->OnCreate();

			AutoFree<IAppManager> appManager = CreateAppManager(mainWindow, *app);
			appManager->Run(hInstanceLock, *app, platform.appControl);
		}
	} proxy;

	proxy.appFactory = &appFactory;

	return Main(hInstance, proxy, title, hLargeIcon, hSmallIcon);
}

int Main(HINSTANCE hInstance, IDirectAppFactory& appFactory, cstr title, HICON hLargeIcon, HICON hSmallIcon)
{
	CoInitializeEx(NULL, COINIT_MULTITHREADED);

	PerformSanityTests();

	struct : public IMainloop
	{
		IDirectAppFactory* appFactory;

		void Invoke(Platform& platform, HANDLE hInstanceLock, IDX11GraphicsWindow& mainWindow) override
		{
			AutoFree<IDirectAppManager> appManager = CreateAppManager(platform, mainWindow, *appFactory);
			appManager->Run(hInstanceLock);
		}
	} proxy;

	proxy.appFactory = &appFactory;

	return Main(hInstance, proxy, title, hLargeIcon, hSmallIcon);
}

namespace Rococo
{
	int M_Platorm_Win64_Main(HINSTANCE hInstance, IAppFactory& factory, cstr title, HICON hLarge, HICON hSmall)
	{
		int errCode = 0;

		try
		{
			errCode = Main(hInstance, factory, title, hLarge, hSmall);
		}
		catch (IException& ex)
		{
			char text[256];
			SafeFormat(text, "%s crashed", title);
			OS::ShowErrorBox(NoParent(), ex, text);
			errCode = ex.ErrorCode();
		}

		return errCode;
	}

	int M_Platorm_Win64_MainDirect(HINSTANCE hInstance, IDirectAppFactory& factory, cstr title, HICON hLarge, HICON hSmall)
	{
		Rococo::OS::SetBreakPoints(Rococo::OS::BreakFlag_All);

		int errCode = 0;

		try
		{
			errCode = Main(hInstance, factory, title, hLarge, hSmall);
		}
		catch (IException& ex)
		{
			char text[256];
			SafeFormat(text, "%s crashed", title);
			OS::ShowErrorBox(NoParent(), ex, text);
			errCode = ex.ErrorCode();
		}

		return errCode;
	}
}