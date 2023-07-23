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
#include <rococo.audio.h>
#include <objbase.h>
#include <rococo.stl.allocators.h>
#include "mplat.components.h"
#include <rococo.gui.retained.ex.h>
#include "mplat.editor.h"
#include "rococo.maths.h"
#include "rococo.ecs.h"

#ifdef _WIN32
# pragma comment(lib, "rococo.util.ex.lib")
# pragma comment(lib, "rococo.audio.lib")
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
		IGuiStackSupervisor* CreateGui(IPublisher& publisher, ISourceCache& cache, IRenderer& renderer, IUtilities& utils);
	}
}

namespace Rococo::Gui
{
	IMPlatGuiCustodianSupervisor* CreateMPlatCustodian(IUtilities& utilities, IRenderer& sysRenderer);
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

		void InitScriptSystem(IO::IInstallation& installation);
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
		platform.plumbing.publisher.Subscribe(this, evUIPopulate);
	}

	~PlatformTabs()
	{
		platform.plumbing.publisher.Unsubscribe(this);
	}


	virtual void ShowVenue(IMathsVisitor& visitor)
	{
		platform.graphics.renderer.ShowWindowVenue(visitor);
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
			venue = platform.graphics.renderer.Venue();
		}
		else if (Eq(pop.name, "overlay.camera"))
		{
			pop.renderElement = this;
			venue = &platform.graphics.camera.Venue();
		}
		else if (Eq(pop.name, "overlay.textures"))
		{
			pop.renderElement = this;
			venue = platform.graphics.renderer.TextureVenue();
		}
		else if (Eq(pop.name, "overlay.meshes"))
		{
			pop.renderElement = this;
			venue = platform.graphics.meshes.Venue();
		}
		else if (Eq(pop.name, "overlay.os"))
		{
			pop.renderElement = this;
			venue = &GetOSVenue();
		}
		else if (Eq(pop.name, "overlay.cache"))
		{
			pop.renderElement = this;
			venue = platform.scripts.sourceCache.Venue();
		}
		else if (Eq(pop.name, "overlay.performance"))
		{
			pop.renderElement = this;
			venue = platform.plumbing.utilities.Venue();
		}
		else
		{
		}
	}

	bool OnKeyboardEvent(const KeyboardEvent& key)  override
	{
		return platform.misc.mathsVisitor.AppendKeyboardEvent(key);
	}

	void OnRawMouseEvent(const MouseEvent& me) override
	{
		platform.misc.mathsVisitor.AppendMouseEvent(me);
	}

	void OnMouseMove(Vec2i, Vec2i, int)  override
	{

	}

	void OnMouseLClick(Vec2i cursorPos, bool clickedDown) override
	{
		if (!clickedDown) platform.misc.mathsVisitor.SelectAtPos(cursorPos);
	}

	void OnMouseRClick(Vec2i, bool)  override
	{

	}

	void Render(IGuiRenderContext& rc, const GuiRect& absRect)  override
	{
		platform.misc.mathsVisitor.Clear();
		if (venue) venue->ShowVenue(platform.misc.mathsVisitor);
		platform.misc.mathsVisitor.Render(rc, absRect, 4);
	}

};

HINSTANCE g_Instance = nullptr;
cstr g_largeIcon = nullptr;
cstr g_smallIcon = nullptr;

ROCOCO_INTERFACE IMainloop
{
	virtual void Invoke(Platform& platform, HANDLE hInstanceLock, IGraphicsWindow& mainWindow) = 0;
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

int GetClampedInt(IConfigSupervisor& config, cstr name, int defaultValue, int minValue, int maxValue)
{
	int result = 0;
	config.TryGetInt(name, result, defaultValue);
	result = clamp(result, minValue, maxValue);
	return result;
}

Rococo::Strings::CLI::CommandLineOption cmdOptionHelp =
{
	"-?"_fstring,
	"List command line options and their help strings. Command line options that end with a colon take an argument with no blankspace between the option and the argument."_fstring
};

Rococo::Strings::CLI::CommandLineOption cmdOptionTitle =
{
	"-title:"_fstring,
	"Assigns a caption to the main window and other elements of the application"_fstring
};

Rococo::Strings::CLI::CommandLineOption cmdOptionFontScale =
{
	"-font.scale"_fstring,
	"If added on the command line, attempts to correct the font for 4k+ monitors"_fstring
};

Rococo::Strings::CLI::CommandLineOption cmdOptionFontFaceName =
{
	"-font.facename:"_fstring,
	"Specified the main window LOGFONT face name. Defaults to consolas"_fstring
};


Rococo::Strings::CLI::CommandLineOptionInt32 cmdOptionInt32_AllocScriptsInitial =
{
	{
		"-alloc.script.initial:"_fstring,
		"decimal int32 specifying the initial length of the script allocator in kilobytes"_fstring
	},
	16384, 0, 1048576
};

Rococo::Strings::CLI::CommandLineOptionInt32 cmdOptionInt32_AllocImagesInitial =
{
	{
		"-alloc.images.initial:"_fstring,
		"decimal int32 specifying the initial length of the image allocator in kilobytes"_fstring
	},
	16384, 0, 1048576
};

Rococo::Strings::CLI::CommandLineOptionInt32 cmdOptionInt32_windowFontSize =
{
	{
		"-font.size:"_fstring,
		"decimal int32 specifying the initial font size for the windows LOGFONT struct. Clamped from 12 to 240. 24 is the default"_fstring
	},
	24, 12, 120
};

const Rococo::Strings::CLI::CommandLineOption* options[] =
{
	&cmdOptionHelp,
	&cmdOptionTitle,
	&cmdOptionFontScale,
	&cmdOptionFontFaceName,
	&cmdOptionInt32_AllocScriptsInitial.spec,
	&cmdOptionInt32_AllocImagesInitial.spec,
	&cmdOptionInt32_windowFontSize.spec
};

void ThrowWhenHelpInformationNeeded()
{
	if (Rococo::Strings::CLI::HasSwitch(cmdOptionHelp))
	{
		AutoFree<IDynamicStringBuilder> sb = Rococo::Strings::CreateDynamicStringBuilder(256);
		for (auto* opt : options)
		{
			sb->Builder().AppendFormat("\tOption: %-24.24s %s\n", opt->prefix.buffer, opt->helpString.buffer);
		}

		fstring s = *sb->Builder();

		Throw(0, "Command line options:\n%s", s.buffer);
	}
}

HANDLE CreateInstanceLock()
{
	U8FilePath exeFile;
	GetModuleFileNameA(nullptr, exeFile.buf, exeFile.CAPACITY);

	U8FilePath eventName = exeFile;
	for (char* p = eventName.buf; *p != 0; p++)
	{
		if (*p == '\\') *p = '#';
	}

	HANDLE hInstanceLock = CreateEventA(nullptr, TRUE, FALSE, eventName);
	if (hInstanceLock == NULL)
	{
		Throw(GetLastError(), "Error creating event: %s", eventName.buf);
	}

	int err = GetLastError();
	if (err == ERROR_ALREADY_EXISTS)
	{
		SetEvent(hInstanceLock);

		if (IsDebuggerPresent())
		{
			ShowMessageBox(Windows::NoParent(), "Application is already running", exeFile, MB_ICONEXCLAMATION);
		}

		SetLastError(err);
		return nullptr;
	}

	return hInstanceLock;
}

struct OutputWindowFormatter : Strings::IVarArgStringFormatter, Strings::IColourOutputControl
{
	int PrintFV(const char* format, va_list args) override
	{
		char buf[4096];
		int length = SafeVFormat(buf, sizeof buf, format, args);
		OutputDebugStringA(buf);
		return length;
	}

	void SetOutputColour(RGBAb) override
	{

	}
};

void GetMainWindowSpec(WindowSpec& ws, HINSTANCE hInstance, IConfigSupervisor& config)
{
	RECT workArea;
	::SystemParametersInfo(SPI_GETWORKAREA, 0, &workArea, 0);

	Vec2i workAreaSpan = { workArea.right - workArea.left, workArea.bottom - workArea.top };

	ws.exStyle = 0;
	ws.style = WS_OVERLAPPEDWINDOW;
	ws.hInstance = hInstance;
	ws.hParentWnd = nullptr;
	ws.messageSink = nullptr;
	ws.minSpan = { 1024, 640 };

	Vec2i initialDS;
	initialDS.x = GetClampedInt(config, "mainwindow.initial.x", 24, 0, workAreaSpan.x - 128);
	initialDS.y = GetClampedInt(config, "mainwindow.initial.y", 24, 0, workAreaSpan.y - 128);

	// Clamp the x and y offsets so that our windows does not appear too far offscreen
	ws.X = workArea.left + initialDS.x;
	ws.Y = workArea.top + initialDS.y;

	Vec2i desktopSpan = GetDesktopSpan();

	Vec2i desktopBorder = { 96, 54 };
	desktopBorder.x = GetClampedInt(config, "mainwindow.desktop.border.width", 96, 0, 384);
	desktopBorder.y = GetClampedInt(config, "mainwindow.desktop.border.height", 54, 0, 216);

	Vec2i windowSpan = desktopSpan - 2 * desktopBorder;

	Vec2i resolvedSpan;
	config.TryGetInt("mainwindow.desktop.window.width", resolvedSpan.x, windowSpan.x);
	config.TryGetInt("mainwindow.desktop.window.height", resolvedSpan.y, windowSpan.y);

	ws.Width = clamp(resolvedSpan.x, 768, desktopSpan.x);
	ws.Height = clamp(resolvedSpan.y, 432, desktopSpan.y);
}

void FormatMainWindowFont(LOGFONTA& font)
{
	Rococo::Strings::CLI::GetCommandLineArgument("font.facename:"_fstring, GetCommandLineA(), font.lfFaceName, sizeof font.lfFaceName, "Consolas");

	Vec2i span = Rococo::Windows::GetDesktopSpan();

	font.lfHeight = Rococo::Strings::CLI::GetClampedCommandLineOption(cmdOptionInt32_windowFontSize);

	if (Rococo::Strings::CLI::HasSwitch(cmdOptionFontScale))
	{
		int32 multiplier = 1;
		if (span.x > 1920)
		{
			multiplier = span.x / 1920;
		}

		font.lfHeight *= multiplier;
	}
}

struct ComponentAutoRelease
{
	~ComponentAutoRelease()
	{
		ECS::ReleaseTablesForIAnimationComponent();
		ECS::ReleaseTablesForIBodyComponent();
		ECS::ReleaseTablesForISkeletonComponent();
	}
};

void LinkComponents(IECSSupervisor& ecs, Platform& platform)
{
	ECS::LinkToECS_IAnimationComponentTable(ecs);
	ECS::LinkToECS_IBodyComponentTable(ecs);
	ECS::LinkToECS_ISkeletonComponentTable(ecs, platform.world.rigs.Skeles());
}

int Main(HINSTANCE hInstance, IMainloop& mainloop, cstr title, HICON hLargeIcon, HICON hSmallIcon)
{
	using namespace Rococo::Components;

	HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	if FAILED(hr)
	{
		Throw(hr, "CoInitializeEx failed");
	}

	ThrowWhenHelpInformationNeeded();

	int initialScriptAllocationKB = Rococo::Strings::CLI::GetClampedCommandLineOption(cmdOptionInt32_AllocScriptsInitial);
		
	// We set up allocators first, as subsystems below depend on it implicitly through global new and delete operators
	// We cannot determine the paramenters by sexy script, since the script library depends on the allocators being set first, so use command line parameters
	AutoFree<IAllocatorSupervisor> scriptAllocator = Memory::CreateBlockAllocator(initialScriptAllocationKB, 0, "mplat-for-sexy");
	Rococo::Memory::SetSexyAllocator(scriptAllocator);

	int initialImageAllocationKB = Rococo::Strings::CLI::GetClampedCommandLineOption(cmdOptionInt32_AllocImagesInitial);
	AutoFree<IAllocatorSupervisor> imageAllocator = Memory::CreateBlockAllocator(initialImageAllocationKB, 0, "mplat-for-images");
	Imaging::SetJpegAllocator(imageAllocator);
	Imaging::SetTiffAllocator(imageAllocator);

	U8FilePath exeFile;
	GetModuleFileNameA(nullptr, exeFile.buf, exeFile.CAPACITY);

	HANDLE hInstanceLock = CreateInstanceLock();
	if (!hInstanceLock)
	{
		return GetLastError();
	}

	HandleManager autoInstanceLock(hInstanceLock);

	LOGFONTA font = { 0 };
	FormatMainWindowFont(font);
	InitRococoWindows(hInstance, hLargeIcon, hSmallIcon, &font, &font);

	AutoFree<IO::IOSSupervisor> os = IO::GetIOS();
	AutoFree<IO::IInstallationSupervisor> installation = CreateInstallation(L"content.indicator.txt", *os);
	AutoFree<IConfigSupervisor> config = CreateConfig();

	AutoFree<Rococo::Events::IPublisherSupervisor> publisher(Events::CreatePublisher());
	os->Monitor(installation->Content());

	AutoFree<IDX11Logger> logger = CreateStandardOutputLogger();
	AutoFree<ISourceCache> sourceCache(CreateSourceCache(*installation, *scriptAllocator));
	
	Rococo::MPlatImpl::InitScriptSystem(*installation);
	
	AutoFree<Rococo::Script::IScriptSystemFactory> ssFactory = CreateScriptSystemFactory_1_5_0_0();
	AutoFree<OS::IAppControlSupervisor> appControl(OS::CreateAppControl());

	OutputWindowFormatter outputWindowFormatter;
	AutoFree<IDebuggerWindow> consoleDebugger = GetConsoleAsDebuggerWindow(outputWindowFormatter, outputWindowFormatter);

	// We need config to control window settings, thus the HWND based debugger window will not be available at this time
	RunMPlatConfigScript(*config, "!scripts/config_mplat.sxy", *ssFactory, EScriptExceptionFlow::Terminate, *consoleDebugger, *sourceCache, *appControl, nullptr, *installation);

	FactorySpec factorySpec;
	factorySpec.hResourceInstance = hInstance;
	factorySpec.largeIcon = hLargeIcon;
	factorySpec.smallIcon = hSmallIcon;
	AutoFree<IDX11Factory> factory = CreateDX11Factory(*installation, *logger, factorySpec);

	WindowSpec ws;
	GetMainWindowSpec(ws, hInstance, *config);
	AutoFree<IGraphicsWindow> mainWindow = factory->CreateDX11Window(ws, true);
	mainWindow->MakeRenderTarget();

	SetWindowTextA(mainWindow->Window(), title);

	AutoFree<IDebuggerEventHandler> debuggerEventHandler(CreateDebuggerEventHandler(*installation, mainWindow->Window()));
	AutoFree<IDebuggerWindow> debuggerWindow(CreateDebuggerWindow(mainWindow->Window(), debuggerEventHandler->GetMenuCallback(), *appControl));

	int32 maxEntities = config->GetInt("mplat.instances.entities.max"_fstring);
	if (maxEntities <= 0) Throw(0, "Int32 \"mplat.instances.entities.max\" defined in '!scripts/config_mplat.sxy' was not positive");

	Audio::AudioConfig audio_config{};
	AutoFree<Audio::IOSAudioAPISupervisor> osAudio = Audio::CreateOSAudio();
	AutoFree<Audio::IAudioInstallationSupervisor> audioInstallation = Audio::CreateAudioInstallation(*installation);
	AutoFree<Audio::IAudioSupervisor> audio = Audio::CreateAudioSupervisor(*audioInstallation, *osAudio, audio_config);

	AutoFree<Rococo::Entities::IRigs> rigs = Rococo::Entities::CreateRigBuilder();
	AutoFree<Graphics::IMeshBuilderSupervisor> meshes = Graphics::CreateMeshBuilder(mainWindow->Renderer());

	AutoFree<IComponentFactory<IParticleSystemComponent>> particleSystemFactory;
	AutoFree<IComponentFactory<IRigsComponent>> rigsFactory;

	ComponentAutoRelease componentReleaser; // Ensure this is created before the ecs system, as component tables must remain valid for the lifetime of the ECS - the ECS references them
	AutoFree<IECSSupervisor> ecs = CreateECS(32_megabytes);
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

	Rococo::Gui::GRConfig grConfig;

	AutoFree<Rococo::Gui::IMPlatGuiCustodianSupervisor> mplat_gcs = Rococo::Gui::CreateMPlatCustodian(*utilities, mainWindow->Renderer());
	AutoFree<Rococo::Gui::IGRSystemSupervisor> GR = Rococo::Gui::CreateGRSystem(grConfig, mplat_gcs->Custodian());

#ifdef _DEBUG
	GR->SetDebugFlags((int) Rococo::Gui::EGRDebugFlags::ThrowWhenPanelIsZeroArea);
#endif

	AutoFree<Rococo::MPEditor::IMPEditorSupervisor> editor = Rococo::MPEditor::CreateMPlatEditor(*GR);

	Platform platform
	{ 
		// Platform graphics
		{ *rendererConfig, mainWindow->Renderer(), *sprites, *gui, *meshes, *instances, *spriteBuilder, *camera, *scene, *GR, *mplat_gcs },

		// Platform os
		{ *os, *installation, *ims, *appControl, mainWindow->Window(), title },

		// Platform scripting
		{ *sourceCache, *debuggerWindow, *ssFactory },

		// Plaform hardware
		{ *keyboard, *audio, *xbox360stick },

		// Platform world
		{ *mobiles, *particles, *rigs, *world, *ecs },

		// Platform data
		{
			*config,
			*archive,
		},

		// Platform plumbing
		{
			*messaging, *publisher, *utilities
		},

		// Platform creator
		{
			*editor
		},

		// Platform miscellaneous interfaces
		{
			*mathsVisitor
		},

		tesselators
	};

	LinkComponents(*ecs, platform);

	editor->SetPlatform(&platform);

	gui->PostConstruct(&platform);
	utilities->SetPlatform(platform);
	messaging->PostCreate(platform);

	PlatformTabs tabs(platform);

	PerformSanityTests();

	mainloop.Invoke(platform, hInstanceLock, *mainWindow);

	return 0;
}

int Main(HINSTANCE hInstance, IAppFactory& appFactory, cstr title, HICON hLargeIcon, HICON hSmallIcon)
{
	struct : public IMainloop
	{
		IAppFactory* appFactory = nullptr;
		IEventCallback<ScriptCompileArgs>* onCompile = nullptr;

		void Invoke(Platform& platform, HANDLE hInstanceLock, IGraphicsWindow& mainWindow) override
		{
			AutoFree<IApp> app(appFactory->CreateApp(platform));

			app->OnCreate();

			AutoFree<IAppManager> appManager = CreateAppManager(mainWindow, *app);
			appManager->Run(hInstanceLock, *app, platform.os.appControl);
		}
	} proxy;

	proxy.appFactory = &appFactory;

	return Main(hInstance, proxy, title, hLargeIcon, hSmallIcon);
}

int Main(HINSTANCE hInstance, IDirectAppFactory& appFactory, cstr title, HICON hLargeIcon, HICON hSmallIcon)
{
	struct : public IMainloop
	{
		IDirectAppFactory* appFactory;

		void Invoke(Platform& platform, HANDLE hInstanceLock, IGraphicsWindow& mainWindow) override
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
			Windows::ShowErrorBox(NoParent(), ex, text);
			errCode = ex.ErrorCode();
		}

		return errCode;
	}

	int M_Platorm_Win64_MainDirect(HINSTANCE hInstance, IDirectAppFactory& factory, cstr title, HICON hLarge, HICON hSmall)
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
			Windows::ShowErrorBox(NoParent(), ex, text);
			errCode = ex.ErrorCode();
		}

		return errCode;
	}
}