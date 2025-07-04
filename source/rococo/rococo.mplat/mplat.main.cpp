#include <rococo.mplat.h>
#include <rococo.io.h>
#include <rococo.os.win32.h>
#include <rococo.window.h>
#include <rococo.sexy.ide.h>
#include <rococo.win32.rendering.h>
#define ROCOCO_USE_SAFE_V_FORMAT
#include <rococo.strings.h>
#include <rococo.imaging.h>
#include <rococo.strings.h>
#include <components/rococo.ecs.h>
#include <rococo.sexy.allocators.h>
#include <rococo.gui.retained.ex.h>
#include <rococo.ide.h>
#include <rococo.audio.h>
#include <rococo.fonts.h>
#include <rococo.maths.h>
#include <rococo.os.h>
#include <rococo.allocators.h>
#include <3D/rococo.material-builder.h>
#include <sexy.script.h>

#include <mplat.to.app.events.inl>
#include "mplat.panel.base.h"
#include "mplat.components.h"
#include "mplat.editor.h"

#include <objbase.h>

#include <vector>
#include <algorithm>
#include <unordered_set>

///////////////////////////////////////////////////////////////////////////////////////////////////

#undef DrawText

#pragma warning (disable : 4250)

using namespace Rococo;
using namespace Rococo::Events;
using namespace Rococo::Windows;
using namespace Rococo::MPlatImpl;
using namespace Rococo::Components;
using namespace Rococo::Graphics;
using namespace Rococo::Strings;

static auto evFileUpdated = "OnFileUpdated"_event;

void PerformSanityTests();

namespace Rococo 
{
	namespace MPlatImpl
	{
		GUI::IPaneContainer* CreatePaneContainer(Platform& platform);
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
	namespace IO
	{
		// If sexmlSection is null or blank a default is chosen
		IO::IShaderMonitor* TryCreateShaderMonitor(cstr sexmlSection, IO::IShaderMonitorEvents& events);
	}
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

	bool OnKeyboardEvent(const KeyboardEventEx& key)  override
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

Rococo::Strings::CLI::CommandLineOption cmdOptionDisableWindowsAssociationInDX11
{
	"-DWA"_fstring,
	"Disables windows association with DX11. Essential for nVidia nSight debugging"_fstring
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

Rococo::Strings::CLI::CommandLineOption cmdOptionPIX =
{
	"-pix"_fstring,
	"Load PIX prerequisites. Required when attaching PIX."_fstring
};

const Rococo::Strings::CLI::CommandLineOption* options[] =
{
	&cmdOptionHelp,
	&cmdOptionPIX,
	&cmdOptionTitle,
	&cmdOptionFontScale,
	&cmdOptionFontFaceName,
	&cmdOptionDisableWindowsAssociationInDX11,
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
	font = { 0 };
	Rococo::Strings::CLI::GetCommandLineArgument("font.facename:"_fstring, GetCommandLineA(), font.lfFaceName, LF_FACESIZE, "Consolas");
	font.lfHeight = Rococo::Strings::CLI::GetClampedCommandLineOption(cmdOptionInt32_windowFontSize);
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

	Rococo::Components::Body::BodyComponentCreationArgs args { platform.graphics.meshes.MeshDictionary()};
	ECS::LinkToECS_IBodyComponentTable(ecs, args);
	ECS::LinkToECS_ISkeletonComponentTable(ecs, platform.world.rigs.Skeles());
}

int Main(HINSTANCE hInstance, IMainloop& mainloop, cstr title, HICON hLargeIcon, HICON hSmallIcon)
{
	using namespace Rococo::Components;

	Vec2i span = Rococo::Windows::GetDesktopSpan();

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

	AutoFree<IShaderOptionsSupervisor> shaderOptions = Construction::CreateShaderOptions();

	AutoFree<IO::IOSSupervisor> os = IO::GetIOS();
	AutoFree<IO::IInstallationSupervisor> installation = CreateInstallation(L"content.indicator.txt", *os);
	AutoFree<IConfigSupervisor> config = CreateConfig();

	AutoFree<Rococo::Events::IPublisherSupervisor> publisher(Events::CreatePublisher());
	os->Monitor(installation->Content());

	AutoFree<IGraphicsLogger> logger = CreateStandardOutputLogger();
	AutoFree<ISourceCache> sourceCache(CreateSourceCache(*installation, *scriptAllocator));

	Rococo::MPlatImpl::InitScriptSystem(*installation);

	AutoFree<Rococo::Script::IScriptSystemFactory> ssFactory = CreateScriptSystemFactory_1_5_0_0();

	struct MonitorEvents : IO::IShaderMonitorEvents, IO::IShaderMonitorEventsProxy
	{
		std::unordered_set<IO::IShaderMonitorEventHook*> hooks;

		void AddHook(IO::IShaderMonitorEventHook* hook) override
		{
			hooks.insert(hook);
		}

		void RemoveHook(IO::IShaderMonitorEventHook* hook) override
		{
			hooks.erase(hook);
		}

		void OnLog(IO::IShaderMonitor& monitor, IO::EShaderLogPriority priority, cstr file, cstr text) override
		{
			for (auto* h : hooks)
			{
				h->OnLog(monitor, priority, file, text);
			}
		}

		void OnModifiedFileSkipped(IO::IShaderMonitor& monitor, cstr text) override
		{
			if (EndsWith(text, ".api.hlsl") || EndsWith(text, "types.hlsl"))
			{
				// An intermediate file, which means dependents have to be recompiled.
				// At the time of writing everything compiles in under a second, so for now
				// just compile everything

				monitor.CompileDirectory(nullptr);
			}
		}
	} shaderEventHandler;

	U8FilePath shaderCompilerConfigSection;
	Format(shaderCompilerConfigSection, "%s-%s", title, ".hlsl.compiler");

	AutoFree<IO::IShaderMonitor> shaderMonitor = IO::TryCreateShaderMonitor(shaderCompilerConfigSection, shaderEventHandler);

	AutoFree<OS::IAppControlSupervisor> appControl(OS::CreateAppControl());

	if (shaderMonitor)
	{
		appControl->AddSysMonitor(*shaderMonitor);
	}

	OutputWindowFormatter outputWindowFormatter;
	AutoFree<IDebuggerWindow> consoleDebugger = GetConsoleAsDebuggerWindow(outputWindowFormatter, outputWindowFormatter);

	// We need config to control window settings, thus the HWND based debugger window will not be available at this time
	RunMPlatConfigScript(shaderOptions->Config(), *config, "!scripts/config_mplat.sxy", *ssFactory, EScriptExceptionFlow::Terminate, *consoleDebugger, *sourceCache, *appControl, nullptr, *installation);

	FactorySpec factorySpec;
	factorySpec.hResourceInstance = hInstance;
	factorySpec.largeIcon = hLargeIcon;
	factorySpec.smallIcon = hSmallIcon;
	factorySpec.preparePix = Rococo::Strings::CLI::HasSwitch(cmdOptionPIX);

	AutoFree<IGraphicsWindowFactory> factory = CreateGraphicsWindowFactory(*installation, *logger, factorySpec, *shaderOptions);

	bool dwa = Rococo::Strings::CLI::HasSwitch(cmdOptionDisableWindowsAssociationInDX11);

	struct WindowEventHandler : Graphics::IWindowEventHandler
	{
		IPublisher& publisher;

		WindowEventHandler(IPublisher& _publisher): publisher(_publisher)
		{

		}

		void OnPostResize(bool isFullscreen, Vec2i span) override
		{			
			WindowResizeEvent ev;
			ev.isFullscreen = isFullscreen;
			ev.span = span;
			publisher.Post(ev, "mainWindow.post_resize"_event, PostQuality::Overwrites);
		}
	} windowEventHandler(*publisher);

	WindowSpec ws;
	GetMainWindowSpec(ws, hInstance, *config);
	AutoFree<IGraphicsWindow> mainWindow = factory->CreateGraphicsWindow(windowEventHandler, ws, !dwa);
	mainWindow->MakeRenderTarget();

	SetWindowTextA(mainWindow->Window(), title);

	AutoFree<IDebuggerWindow> debuggerWindow(CreateDebuggerWindow(mainWindow->Window(), *appControl, *installation));

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
	AutoFree<Graphics::IMaterialBuilderSupervisor> materialBuilder = Graphics::Construction::CreateMaterialsBuilder(mainWindow->Renderer(), *publisher);
	AutoFree<Entities::IMobilesSupervisor> mobiles = Entities::CreateMobilesSupervisor(*ecs);
	AutoFree<Graphics::ICameraSupervisor> camera = Graphics::CreateCamera(*mobiles, mainWindow->Renderer());
	AutoFree<Graphics::ISceneSupervisor> scene = Graphics::CreateScene(*ecs, *camera, *rigs);
	AutoFree<IKeyboardSupervisor> keyboard = CreateKeyboardSupervisor(mainWindow->Window());
	AutoFree<Graphics::ISpriteBuilderSupervisor> spriteBuilder = Graphics::CreateSpriteBuilderSupervisor(mainWindow->Renderer());
	AutoFree<Graphics::IRimTesselatorSupervisor> rimTesselator = Graphics::CreateRimTesselator();
	AutoFree<Graphics::IRodTesselatorSupervisor> rodTesselator = Graphics::CreateRodTesselator(*meshes);
	AutoFree<Entities::IParticleSystemSupervisor> particles = Entities::CreateParticleSystem(mainWindow->Renderer());
	AutoFree<Graphics::IRendererConfigSupervisor> rendererConfig = Graphics::CreateRendererConfig(mainWindow->Renderer());
	AutoFree<IUtilitiesSupervisor> utilities = CreateUtilities(*installation, mainWindow->Renderer());
	AutoFree<IMathsVisitorSupervisor> mathsVisitor = CreateMathsVisitor(*utilities, *publisher);
	AutoFree<IGuiStackSupervisor> gui = CreateGui(*publisher, *sourceCache, mainWindow->Renderer(), *utilities);
	AutoFree<Graphics::IMessagingSupervisor> messaging = Graphics::CreateMessaging();

	Tesselators tesselators{ *rimTesselator, *rodTesselator };

	AutoFree<Joysticks::IJoystick_XBOX360_Supervisor> xbox360stick = Joysticks::CreateJoystick_XBox360Proxy();
	AutoFree<IInstallationManagerSupervisor> ims = Rococo::MPlatImpl::CreateIMS(*installation);
	AutoFree<IArchiveSupervisor> archive = Rococo::CreateArchive();
	AutoFree<IWorldSupervisor> world = Rococo::CreateWorld(*meshes, *ecs);
	AutoFree<Graphics::ISpritesSupervisor> sprites = Rococo::Graphics::CreateSpriteTable(mainWindow->Renderer());

	Rococo::Gui::GRConfig grConfig;

	AutoFree<Rococo::Gui::IMPlatGuiCustodianSupervisor> mplat_gcs = Rococo::Gui::CreateMPlatCustodian(*utilities, mainWindow->Renderer());
	AutoFree<Rococo::Gui::IGRSystemSupervisor> GR = Rococo::Gui::CreateGRSystem(grConfig, mplat_gcs->Custodian());

	AutoFree<Rococo::Graphics::ISoftBoxBuilderSupervisor> softBoxBuilder = Rococo::Graphics::CreateSoftboxBuilder();

#ifdef _DEBUG
	GR->SetDebugFlags((int) Rococo::Gui::EGRDebugFlags::ThrowWhenPanelIsZeroArea);
#endif

	AutoFree<Rococo::MPEditor::IMPEditorSupervisor> editor = Rococo::MPEditor::CreateMPlatEditor(*GR);

	struct PanelCompilationHandlerProxy : IScriptCompilationEventHandler, IDesignator<IScriptCompilationEventHandler>, IScriptEnumerator
	{
		IScriptCompilationEventHandler* target = nullptr;
		void OnCompile(ScriptCompileArgs& args) override
		{
			if (target) target->OnCompile(args);
		}

		IScriptEnumerator* ImplicitIncludes() override
		{
			return this;
		}

		size_t Count() const override
		{
			IScriptEnumerator* inner = target ? target->ImplicitIncludes() : nullptr;
			return inner ? inner->Count() : 0;
		}

		cstr ResourceName(size_t index) const override
		{
			IScriptEnumerator* inner = target ? target->ImplicitIncludes() : nullptr;
			return inner ? inner->ResourceName(index) : nullptr;
		}

		void Designate(IScriptCompilationEventHandler* object)
		{
			target = object;
		}
	} panelCompilationHandler;

	AutoFree<ISubsystemsSupervisor> subsystems = CreateSubsystemMonitor();

	Platform platform
	{ 
		// Platform graphics
		{ *rendererConfig, mainWindow->Renderer(), *sprites, *gui, *meshes, *materialBuilder, *softBoxBuilder, *spriteBuilder, *camera, *scene, *GR, *mplat_gcs, shaderOptions->Config(), shaderEventHandler},

		// Platform os
		{ *os, *installation, *ims, *appControl, mainWindow->Window(), title },

		// Platform scripting
		{ panelCompilationHandler, panelCompilationHandler, *sourceCache, *debuggerWindow, *ssFactory },

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
			*mathsVisitor,
			*subsystems
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

	struct PlatformSubsystem: ISubsystem
	{
		ISubsystemsSupervisor& subsystems;
		Platform& platform;

		PlatformSubsystem(ISubsystemsSupervisor& _subsystems, Platform& _platform): subsystems(_subsystems), platform(_platform)
		{

		}

		~PlatformSubsystem()
		{
			subsystems.Monitor().Unregister(*this);
		}

		void Register()
		{
			auto platformId = subsystems.Monitor().RegisterAtRoot(*this);
			RegisterSubsystems(subsystems, platform, platformId);
		}

		[[nodiscard]] cstr SubsystemName() const override
		{
			return "Platform";
		}

		Reflection::IReflectionTarget* ReflectionTarget() override
		{
			return nullptr;
		}

	} platformSubsystem(*subsystems, platform);

	platformSubsystem.Register();

	mainloop.Invoke(platform, hInstanceLock, *mainWindow);

	SaveAsSexML("mhost.editor", editor->ReflectionTarget());

	return 0;
}

int Main(HINSTANCE hInstance, IAppFactory& appFactory, cstr title, HICON hLargeIcon, HICON hSmallIcon)
{
	struct : public IMainloop
	{
		IAppFactory* appFactory = nullptr;
		IScriptCompilationEventHandler* onCompile = nullptr;

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

		Rococo::OS::InitRococoOS();

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

		Rococo::OS::TerminateRococoOS();

		return errCode;
	}

	int M_Platorm_Win64_MainDirect(HINSTANCE hInstance, IDirectAppFactory& factory, cstr title, HICON hLarge, HICON hSmall)
	{
		int errCode = 0;

		Rococo::OS::InitRococoOS();

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

		Rococo::OS::TerminateRococoOS();

		return errCode;
	}
}