#include <rococo.mplat.h>
#include <rococo.audio.h>
#include "mplat.landscapes.h"
#include <sexy.script.h>
#include <sexy.vm.cpu.h>
#include <rococo.sexy.api.h>
#include <rococo.io.h>
#include <ctype.h>
#include <string.h>

namespace Rococo // declarations herein are to help intellisense do its job.
{
	struct IKeyboard;	

	namespace GUI
	{
		struct IPaneBuilder;
	}

	namespace Configuration
	{
		struct IConfig;
	}

	namespace Entities
	{
		struct IMobiles;
		struct IParticleSystem;
		struct IInstances;
		struct IRigs;
	}

	namespace Graphics
	{
		struct IFieldTesselator;
		struct IRendererConfig;
		struct ITextTesselator;
		struct IMessaging;
		struct IQuadStackTesselator;
		struct IRodTesselator;
		struct IRimTesselator;
		struct ISpriteBuilder;
		struct ISceneBuilder;
		struct ICamera;
		struct IMobiles;

		IFieldTesselator* CreateFieldTesselator();
	}
}

Rococo::Graphics::ISprites* FactoryConstructRococoGraphicsSprites(Rococo::Graphics::ISprites* s)
{
	return s;
}

Rococo::Entities::IRigBuilder* FactoryConstructRococoEntitiesRigBuilder(Rococo::Entities::IRigs* rigs)
{
	return &rigs->Builder();
}

Rococo::IWorldBuilder* FactoryConstructRococoWorldBuilder(Rococo::Platform* platform)
{
	return &platform->world.worldBuilder;
}

Rococo::IArchive* FactoryConstructRococoGetArchive(Rococo::Platform * platform)
{
	return &platform->data.archive;
}

Rococo::Graphics::IRendererConfig* FactoryConstructRococoGraphicsRendererConfig(Rococo::Platform* platform)
{
	return &platform->graphics.rendererConfig;
}

Rococo::Graphics::IMessaging* FactoryConstructRococoGraphicsMessaging(Rococo::Platform* platform)
{
	return &platform->plumbing.messaging;
}

Rococo::Graphics::ITextTesselator* FactoryConstructRococoGraphicsTextTesselator(Rococo::Platform* platform)
{
	return &platform->plumbing.utilities.GetTextTesselator();
}

Rococo::Graphics::IQuadStackTesselator* FactoryConstructRococoGraphicsQuadStackTesselator(Rococo::Graphics::IQuadStackTesselator*)
{
	return Rococo::Graphics::CreateQuadStackTesselator();
}

Rococo::Graphics::IHQFonts* FactoryConstructRococoGraphicsHQFonts(Rococo::Platform* platform)
{
	return &platform->plumbing.utilities.GetHQFonts();
}

Rococo::Graphics::IRodTesselator* FactoryConstructRococoGraphicsRodTesselator(Rococo::Platform* platform)
{
	return Rococo::Graphics::CreateRodTesselator(platform->graphics.meshes);
}

Rococo::Graphics::ILandscapeTesselator*  FactoryConstructRococoGraphicsLandscapeTesselator(Rococo::Platform *nceContext)
{
	return Rococo::Graphics::CreateLandscapeTesselator(nceContext->graphics.meshes);
}

Rococo::Graphics::IFieldTesselator* FactoryConstructRococoGraphicsFieldTesselator(Rococo::Graphics::IFieldTesselator*)
{
	return Rococo::Graphics::CreateFieldTesselator();
}

Rococo::Graphics::IRimTesselator* FactoryConstructRococoGraphicsRimTesselator(Rococo::Graphics::IRimTesselator* t)
{
	return t;
}

Rococo::Configuration::IConfig* FactoryConstructRococoConfigurationConfig(Rococo::Configuration::IConfig* c)
{
   return c;
}

Rococo::Graphics::ISpriteBuilder* FactoryConstructRococoGraphicsSpriteBuilder(Rococo::Graphics::ISpriteBuilder* sb)
{
   return sb;
}

Rococo::IKeyboard* FactoryConstructRococoKeyboard(Rococo::IKeyboard* k)
{
   return k;
}

Rococo::Graphics::ISceneBuilder* FactoryConstructRococoGraphicsSceneBuilder(Rococo::Graphics::ISceneBuilder* sb)
{
   return sb;
}

Rococo::Graphics::ICamera* FactoryConstructRococoGraphicsCamera(Rococo::Graphics::ICamera* c)
{
   return c;
}

Rococo::Entities::IMobiles* FactoryConstructRococoEntitiesMobiles(Rococo::Entities::IMobiles* m)
{
   return m;
}

Rococo::GUI::IPaneBuilder* FactoryConstructRococoGUIPaneBuilder(Rococo::GUI::IPaneBuilder* pb)
{
   return pb;
}

Rococo::Entities::IParticleSystem* FactoryConstructRococoEntitiesParticleSystem(Rococo::Platform* platform)
{
	return &platform->world.particles;
}

Rococo::Entities::IInstances* FactoryConstructRococoEntitiesInstances(Rococo::Entities::IInstances* ins)
{
   return ins;
}

Rococo::Graphics::IMeshBuilder* FactoryConstructRococoGraphicsMeshBuilder(Rococo::Graphics::IMeshBuilder* mb)
{
   return mb;
}

Rococo::IInstallationManager* FactoryConstructRococoInstallation(Rococo::Platform* p)
{
	return &p->os.installationManager;
}

Rococo::Audio::IAudio* FactoryConstructRococoAudioGetAudio(Rococo::Audio::IAudio* a)
{
	return a;
}

#include <../rococo.mplat/code-gen/rococo.sxh.inl>

#include <rococo.sexy.ide.h>
#include <rococo.strings.h>

inline ObjectStub* InterfaceToInstance(InterfacePointer i)
{
	auto* p = ((uint8*)i) + (*i)->OffsetToInstance;
	auto* obj = (ObjectStub*)p;
	return obj;
}

static void NativeEnumerateFiles(NativeCallEnvironment& nce)
{
	auto& platform = *(Platform*)nce.context;

	InterfacePointer ipFilter;
	ReadInput(0, ipFilter, nce);

	ArchetypeCallback ac;
	ReadInput(1, ac, nce);

	auto* stubFilter = InterfaceToInstance(ipFilter);

	auto& sc = (const CStringConstant&) *stubFilter;

	if (sc.length == 0 || sc.pointer == nullptr)
	{
		Throw(0, "MPlat.NativeEnumerateFiles. String argument was blank");
	}

	struct : IEventCallback<IO::FileItemData>
	{
		IPublicScriptSystem* ss;
		IO::IInstallation* installation;
		Rococo::Script::ArchetypeCallback callback = { 0 };

		wchar_t sysRoot[Rococo::IO::MAX_PATHLEN];

		void OnEvent(IO::FileItemData& file) override
		{
			U8FilePath pingPath;
			installation->ConvertSysPathToPingPath(file.fullPath, pingPath);

			auto& SS = (IScriptSystem&)*ss;
			auto* constant = SS.GetStringReflection(SS.GetPersistentString(pingPath));
			InterfacePointer pPingPath = (InterfacePointer)(((uint8*)constant) + ObjectStub::BYTECOUNT_INSTANCE_TO_INTERFACE0);
			
			ss->DispatchToSexyClosure(pPingPath, callback);
		}
	} dispatchToSexyClosure;

	dispatchToSexyClosure.ss = &nce.ss;
	dispatchToSexyClosure.callback = ac;
	dispatchToSexyClosure.installation = &platform.os.installation;

	WideFilePath sysPath;
	platform.os.installation.ConvertPingPathToSysPath((cstr)sc.pointer, sysPath);

	SafeFormat(dispatchToSexyClosure.sysRoot, Rococo::IO::MAX_PATHLEN, L"%s", sysPath.buf);
	Rococo::OS::MakeContainerDirectory(dispatchToSexyClosure.sysRoot);

	Rococo::IO::ForEachFileInDirectory(sysPath, dispatchToSexyClosure, true);
}

const char* s_MplatImplicitIncludes[] =
{
	"!scripts/interop/rococo/mplat/mplat_sxh.sxy",
	"!scripts/interop/rococo/mplat/mplat_gui_sxh.sxy",
	"!scripts/interop/rococo/mplat/mplat_types.sxy",
	"!scripts/interop/rococo/mplat/types.sxy",
	"!scripts/interop/rococo/audio/audio_types.sxy",
	"!scripts/interop/rococo/audio/rococo.audio_sxh.sxy"
};

struct MPlatImplicitIncludes : IScriptEnumerator
{
	size_t Count() const override
	{
		return sizeof s_MplatImplicitIncludes / sizeof(char*);
	}

	cstr ResourceName(size_t index) const override
	{
		return s_MplatImplicitIncludes[index];
	}
} s_MplatImplicitIncludeEnumerator;

namespace Rococo
{
	using namespace Rococo::Windows;

	namespace MPlatImpl
	{
		bool QueryYesNo(IWindow& ownerWindow, cstr message);

		void InitScriptSystem(IO::IInstallation& installation)
		{
			WideFilePath srcpath;
			Format(srcpath, L"%sscripts\\native\\", installation.Content());

			Rococo::Script::SetDefaultNativeSourcePath(srcpath);
		}

		void RunEnvironmentScriptImpl(ScriptPerformanceStats& stats, Platform& platform, IScriptEnumerator* implicitIncludes, IScriptCompilationEventHandler& _onScriptEvent, const char* name, bool addPlatform, bool shutdownOnFail, bool trace, int id, IEventCallback<cstr>* onScriptCrash, StringBuilder* declarationBuilder)
		{
			IScriptEnumerator& usedIncludes = implicitIncludes ? *implicitIncludes : s_MplatImplicitIncludeEnumerator;
			struct ScriptContext : public IScriptCompilationEventHandler, public IDE::IScriptExceptionHandler, public ISecuritySystem
			{
				Platform& platform;
				IScriptCompilationEventHandler& onScriptEvent;
				bool shutdownOnFail = false;
				IEventCallback<cstr>* onScriptCrash = nullptr;
				StringBuilder* declarationBuilder = nullptr;

				void Free() override
				{

				}

				void ValidateSafeToWrite(IPublicScriptSystem& ss, cstr pathname)
				{
					Rococo::MPlatImpl::ValidateSafePathToWrite(platform.os.installation, ss, pathname);
				}

				IDE::EScriptExceptionFlow GetScriptExceptionFlow(cstr source, cstr msg) override
				{
					char fmsg[256];
					SafeFormat(fmsg, "%s: %s", source, msg);
					if (onScriptCrash) onScriptCrash->OnEvent(fmsg);
					platform.os.ios.FireUnstable();
					return IDE::EScriptExceptionFlow::Retry;
				}

				void OnCompile(ScriptCompileArgs& args) override
				{
					args.ss.SetSecurityHandler(*this);

					onScriptEvent.OnCompile(args);

					if (addPlatform)
					{
						Audio::DLL_AddNativeCalls_RococoAudioIAudio(args.ss, &platform.hardware.audio);
						Entities::AddNativeCalls_RococoEntitiesIRigBuilder(args.ss, &platform.world.rigs);
						Graphics::AddNativeCalls_RococoGraphicsIMeshBuilder(args.ss, &platform.graphics.meshes);
						Entities::AddNativeCalls_RococoEntitiesIInstances(args.ss, &platform.graphics.instances);
						Entities::AddNativeCalls_RococoEntitiesIMobiles(args.ss, &platform.world.mobiles);
						Graphics::AddNativeCalls_RococoGraphicsICamera(args.ss, &platform.graphics.camera);
						Graphics::AddNativeCalls_RococoGraphicsISceneBuilder(args.ss, &platform.graphics.scene.Builder());
						Graphics::AddNativeCalls_RococoGraphicsISpriteBuilder(args.ss, &platform.graphics.spriteBuilder);
						Graphics::AddNativeCalls_RococoGraphicsISprites(args.ss, &platform.graphics.sprites);
						Graphics::AddNativeCalls_RococoGraphicsIRimTesselator(args.ss, &platform.tesselators.rim);
						Graphics::AddNativeCalls_RococoGraphicsIFieldTesselator(args.ss, nullptr);
						Graphics::AddNativeCalls_RococoGraphicsIQuadStackTesselator(args.ss, nullptr);
						Graphics::AddNativeCalls_RococoGraphicsIRodTesselator(args.ss, &platform);
						Graphics::AddNativeCalls_RococoGraphicsITextTesselator(args.ss, &platform);
						Graphics::AddNativeCalls_RococoGraphicsIRendererConfig(args.ss, &platform);
						Graphics::AddNativeCalls_RococoGraphicsIMessaging(args.ss, &platform);
						Graphics::AddNativeCalls_RococoGraphicsILandscapeTesselator(args.ss, &platform);
						AddNativeCalls_RococoIKeyboard(args.ss, &platform.hardware.keyboard);
						Entities::AddNativeCalls_RococoEntitiesIParticleSystem(args.ss, &platform);
						Graphics::AddNativeCalls_RococoGraphicsIHQFonts(args.ss, &platform);
						Rococo::AddNativeCalls_RococoIInstallationManager(args.ss, &platform);
						Rococo::Configuration::AddNativeCalls_RococoConfigurationIConfig(args.ss, &platform.data.config);
						Rococo::AddNativeCalls_RococoIArchive(args.ss, &platform);
						Rococo::AddNativeCalls_RococoIWorldBuilder(args.ss, &platform);

						const INamespace& ns = args.ss.AddNativeNamespace("MPlat.OS");
						args.ss.AddNativeCall(ns, NativeEnumerateFiles, &platform, "EnumerateFiles (Sys.Type.IString filter)(MPlat.OnFileName callback)->", __FUNCTION__, __LINE__);
					}
				}

				ScriptContext(Platform& _platform, IScriptCompilationEventHandler& _onScriptEvent, IEventCallback<cstr>* _onScriptCrash) :
					platform(_platform), onScriptEvent(_onScriptEvent), onScriptCrash(_onScriptCrash) {}

				void Execute(cstr name, IScriptEnumerator& implicitIncludes, ScriptPerformanceStats& stats, bool trace, int32 id)
				{
					try
					{
						IDE::ExecuteSexyScriptLoop(stats,
							4096_kilobytes, 
							platform.scripts.ssFactory,
							platform.scripts.sourceCache,
							implicitIncludes,
							platform.scripts.debuggerWindow,
							name, 
							id,
							*this, 
							*this, 
							platform.os.appControl,
							trace,
							declarationBuilder);
					}
					catch (IException&)
					{
						if (shutdownOnFail) platform.os.appControl.ShutdownApp();
						throw;
					}
				}

				bool addPlatform;
			} sc(platform, _onScriptEvent, onScriptCrash);

			sc.addPlatform = addPlatform;
			sc.shutdownOnFail = shutdownOnFail;
			sc.declarationBuilder = declarationBuilder;

			sc.Execute(name, usedIncludes, stats, trace, id);
		}

		void RunBareScript(
			ScriptPerformanceStats& stats,
			IScriptCompilationEventHandler& _onScriptEvent, 
			IDE::EScriptExceptionFlow flow,
			const char* name,
			int id,
			IScriptSystemFactory& ssf,
			IDebuggerWindow& debugger,
			ISourceCache& sources,
			IScriptEnumerator& implicitIncludes,
			OS::IAppControl& appControl,
			StringBuilder* declarationBuilder,
			IO::IInstallation& installation
		)
		{
			struct ScriptContext : public IScriptCompilationEventHandler, public IDE::IScriptExceptionHandler, public ISecuritySystem
			{
				IScriptCompilationEventHandler& onScriptEvent;
				bool shutdownOnFail;
				StringBuilder* declarationBuilder;
				IDE::EScriptExceptionFlow flow;
				IO::IInstallation& installation;

				void Free() override
				{

				}

				void ValidateSafeToWrite(IPublicScriptSystem& ss, cstr pathname) override
				{
					ValidateSafePathToWrite(installation, ss, pathname);
				}

				IDE::EScriptExceptionFlow GetScriptExceptionFlow(cstr source, cstr message) override
				{
					UNUSED(source);
					UNUSED(message);
					return flow;
				}

				void OnCompile(ScriptCompileArgs& args) override
				{
					args.ss.SetSecurityHandler(*this);
					onScriptEvent.OnCompile(args);
				}

				ScriptContext(IScriptCompilationEventHandler& _onScriptEvent, IDE::EScriptExceptionFlow argFlow, IO::IInstallation& _installation) :
					onScriptEvent(_onScriptEvent), flow(argFlow), installation(_installation) {}

				void Execute(cstr name,
					ScriptPerformanceStats stats,
					int32 id,
					IScriptSystemFactory& ssf, 
					IDebuggerWindow& debugger, 
					ISourceCache& sources,
					IScriptEnumerator& implicitIncludes,
					OS::IAppControl& appControl)
				{
					try
					{
						IDE::ExecuteSexyScriptLoop(stats,
							1024_kilobytes,
							ssf,
							sources,
							implicitIncludes,
							debugger,
							name,
							id,
							*this,
							*this,
							appControl,
							false,
							declarationBuilder);
					}
					catch (IException& ex)
					{
						Throw(ex.ErrorCode(), "%s: %s", name, ex.Message());
					}
				}

			} sc(_onScriptEvent, flow, installation);

			sc.declarationBuilder = declarationBuilder;

			sc.Execute(name, stats, id, ssf, debugger, sources, implicitIncludes, appControl);
		}

		void RunMPlatConfigScript(OUT Configuration::IConfig& config, cstr scriptName,
			Script::IScriptSystemFactory& ssf,
			Windows::IDE::EScriptExceptionFlow flow,
			IDebuggerWindow& debugger,
			ISourceCache& sources,
			OS::IAppControl& appControl,
			StringBuilder* declarationBuilder,
			IO::IInstallation& installation
		)
		{
			struct : IScriptCompilationEventHandler
			{
				Configuration::IConfig* config = nullptr;
				void OnCompile(ScriptCompileArgs& args) override
				{
					Rococo::Script::AddNativeCallSecurity_ToSysNatives(args.ss);
					Rococo::Script::AddNativeCallSecurity(args.ss, "Rococo.Configuration.Native", "!scripts/interop/rococo/mplat/mplat_config_sxh.sxy");
					Rococo::Configuration::AddNativeCalls_RococoConfigurationIConfig(args.ss, config);
				}
			} onCompile;

			onCompile.config = &config;

			struct : IScriptEnumerator
			{
				size_t Count() const override
				{
					return 0;
				}

				cstr ResourceName(size_t) const override
				{
					return nullptr;
				}
			} noImplicits;

			ScriptPerformanceStats stats = { 0 };
			RunBareScript(
				stats, onCompile, flow, scriptName, 0,
				ssf, debugger, sources, noImplicits, appControl, declarationBuilder, installation
			);
		}

		void ValidateSafePathToWrite(IO::IInstallation& installation, IPublicScriptSystem& ss, cstr pathname)
		{
			UNUSED(ss);

			if (pathname == nullptr || *pathname == 0)
			{
				Throw(0, "Blank pathname");
			}

			WideFilePath wPath;
			Assign(wPath, pathname);

			U8FilePath pingPath;
			installation.ConvertSysPathToPingPath(wPath, pingPath);

			for (char* c = pingPath.buf; *c != 0; c++)
			{
				if (isalpha(*c))
				{
					*c = (char)tolower(*c);
				}
			}

			if (strstr(pingPath, "/native/") != nullptr)
			{
				Throw(0, "Security Error: request to write to %s refused. Path had /native/ subcomponent.", pingPath.buf);
			}

			if (EndsWith(pingPath, ".sxyz"))
			{
				Throw(0, "Security Error: request to write to %s refused. Path had extension .sxyz, which is reserved for packages", pingPath.buf);
			}
		}
	} // M
} // Rococo