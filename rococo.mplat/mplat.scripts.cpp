#include <rococo.mplat.h>
#include "mplat.landscapes.h"
#include <sexy.script.h>
#include <sexy.vm.cpu.h>

namespace Rococo // declarations herein are to help intellisense do its job.
{
	struct IConfig;
	struct IKeyboard;	
	struct IPaneBuilder;

	namespace Audio
	{
		struct ILegacySoundControl;
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
		struct ISprites;
		struct ISceneBuilder;
		struct ICamera;
		struct IMobiles;

		IFieldTesselator* CreateFieldTesselator();
	}
}

Rococo::Entities::IRigBuilder* FactoryConstructRococoEntitiesRigBuilder(Rococo::Entities::IRigs* rigs)
{
	return &rigs->Builder();
}

Rococo::IWorldBuilder* FactoryConstructRococoWorldBuilder(Rococo::Platform* platform)
{
	return &platform->worldBuilder;
}

Rococo::IArchive* FactoryConstructRococoGetArchive(Rococo::Platform * platform)
{
	return &platform->archive;
}

Rococo::Graphics::IRendererConfig* FactoryConstructRococoGraphicsRendererConfig(Rococo::Platform* platform)
{
	return &platform->rendererConfig;
}

Rococo::Graphics::IMessaging* FactoryConstructRococoGraphicsMessaging(Rococo::Platform* platform)
{
	return &platform->messaging;
}

Rococo::Graphics::ITextTesselator* FactoryConstructRococoGraphicsTextTesselator(Rococo::Platform* platform)
{
	return &platform->utilities.GetTextTesselator();
}

Rococo::Graphics::IQuadStackTesselator* FactoryConstructRococoGraphicsQuadStackTesselator(Rococo::Graphics::IQuadStackTesselator* _context)
{
	return Rococo::Graphics::CreateQuadStackTesselator();
}

Rococo::Graphics::IHQFonts* FactoryConstructRococoGraphicsHQFonts(Rococo::Platform* platform)
{
	return &platform->utilities.GetHQFonts();
}

Rococo::Graphics::IRodTesselator* FactoryConstructRococoGraphicsRodTesselator(Rococo::Platform* platform)
{
	return Rococo::Graphics::CreateRodTesselator(platform->meshes);
}

Rococo::Graphics::ILandscapeTesselator*  FactoryConstructRococoGraphicsLandscapeTesselator(Rococo::Platform *nceContext)
{
	return Rococo::Graphics::CreateLandscapeTesselator(nceContext->meshes);
}

Rococo::Graphics::IFieldTesselator* FactoryConstructRococoGraphicsFieldTesselator(Rococo::Graphics::IFieldTesselator* _context)
{
	return Rococo::Graphics::CreateFieldTesselator();
}

Rococo::Graphics::IRimTesselator* FactoryConstructRococoGraphicsRimTesselator(Rococo::Graphics::IRimTesselator* _context)
{
	return _context;
}

Rococo::IConfig* FactoryConstructRococoConfig(Rococo::IConfig* _context)
{
   return _context;
}

Rococo::Graphics::ISprites* FactoryConstructRococoGraphicsSprites(Rococo::Graphics::ISprites* _context)
{
   return _context;
}

Rococo::IKeyboard* FactoryConstructRococoKeyboard(Rococo::IKeyboard* _context)
{
   return _context;
}

Rococo::Graphics::ISceneBuilder* FactoryConstructRococoGraphicsSceneBuilder(Rococo::Graphics::ISceneBuilder* _context)
{
   return _context;
}

Rococo::Graphics::ICamera* FactoryConstructRococoGraphicsCamera(Rococo::Graphics::ICamera* _context)
{
   return _context;
}

Rococo::Entities::IMobiles* FactoryConstructRococoEntitiesMobiles(Rococo::Entities::IMobiles* _context)
{
   return _context;
}

Rococo::IPaneBuilder* FactoryConstructRococoPaneBuilder(Rococo::IPaneBuilder* _context)
{
   return _context;
}

Rococo::Entities::IParticleSystem* FactoryConstructRococoEntitiesParticleSystem(Rococo::Platform* platform)
{
	return &platform->particles;
}

Rococo::Entities::IInstances* FactoryConstructRococoEntitiesInstances(Rococo::Entities::IInstances* ins)
{
   return ins;
}

Rococo::Graphics::IMeshBuilder* FactoryConstructRococoGraphicsMeshBuilder(Rococo::Graphics::IMeshBuilder* _context)
{
   return _context;
}

Rococo::Audio::ILegacySoundControl* FactoryConstructRococoAudioLegacySoundControl(Rococo::Platform* platform)
{
	return &platform->legacySoundControl;
}

Rococo::IInstallationManager* FactoryConstructRococoInstallation(Rococo::Platform* p)
{
	return &p->installationManager;
}

Rococo::Audio::IAudio* FactoryConstructRococoAudioGetAudio(Rococo::Audio::IAudio* a)
{
	return a;
}

#include <..\rococo.mplat\mplat.sxh.inl>

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

	auto *sf = nce.cpu.SF();

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
		IInstallation* installation;
		Rococo::Script::ArchetypeCallback callback;

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
	dispatchToSexyClosure.installation = &platform.installation;

	WideFilePath sysPath;
	platform.installation.ConvertPingPathToSysPath((cstr)sc.pointer, sysPath);

	SafeFormat(dispatchToSexyClosure.sysRoot, Rococo::IO::MAX_PATHLEN, L"%s", sysPath.buf);
	Rococo::OS::MakeContainerDirectory(dispatchToSexyClosure.sysRoot);

	Rococo::IO::ForEachFileInDirectory(sysPath, dispatchToSexyClosure, true);
}

namespace Rococo
{
	using namespace Rococo::Windows;

	namespace MPlatImpl
	{
		bool QueryYesNo(IWindow& ownerWindow, cstr message);

		void InitScriptSystem(IInstallation& installation)
		{
			WideFilePath srcpath;
			Format(srcpath, L"%sscripts\\native\\", installation.Content());

			Rococo::Script::SetDefaultNativeSourcePath(srcpath);
		}

		void RunEnvironmentScript(ScriptPerformanceStats& stats, Platform& platform, IEventCallback<ScriptCompileArgs>& _onScriptEvent, const char* name, bool addPlatform, bool shutdownOnFail, bool trace, int id, IEventCallback<cstr>* onScriptCrash)
		{
			struct ScriptContext : public IEventCallback<ScriptCompileArgs>, public IDE::IScriptExceptionHandler
			{
				Platform& platform;
				IEventCallback<ScriptCompileArgs>& onScriptEvent;
				bool shutdownOnFail;
				IEventCallback<cstr>* onScriptCrash;

				void Free() override
				{

				}

				IDE::EScriptExceptionFlow GetScriptExceptionFlow(cstr source, cstr message) override
				{
					if (onScriptCrash) onScriptCrash->OnEvent(source);
					platform.os.FireUnstable();
					return IDE::EScriptExceptionFlow_Retry;
				}

				void OnEvent(ScriptCompileArgs& args) override
				{
					if (addPlatform)
					{
						Entities::AddNativeCalls_RococoEntitiesIRigBuilder(args.ss, &platform.rigs);
						Graphics::AddNativeCalls_RococoGraphicsIMeshBuilder(args.ss, &platform.meshes);
						Entities::AddNativeCalls_RococoEntitiesIInstances(args.ss, &platform.instances);
						Entities::AddNativeCalls_RococoEntitiesIMobiles(args.ss, &platform.mobiles);
						Graphics::AddNativeCalls_RococoGraphicsICamera(args.ss, &platform.camera);
						Graphics::AddNativeCalls_RococoGraphicsISceneBuilder(args.ss, &platform.scene.Builder());
						Graphics::AddNativeCalls_RococoGraphicsISprites(args.ss, &platform.sprites);
						Graphics::AddNativeCalls_RococoGraphicsIRimTesselator(args.ss, &platform.tesselators.rim);
						Graphics::AddNativeCalls_RococoGraphicsIFieldTesselator(args.ss, nullptr);
						Graphics::AddNativeCalls_RococoGraphicsIQuadStackTesselator(args.ss, nullptr);
						Graphics::AddNativeCalls_RococoGraphicsIRodTesselator(args.ss, &platform);
						Graphics::AddNativeCalls_RococoGraphicsITextTesselator(args.ss, &platform);
						Graphics::AddNativeCalls_RococoGraphicsIRendererConfig(args.ss, &platform);
						Graphics::AddNativeCalls_RococoGraphicsIMessaging(args.ss, &platform);
						Graphics::AddNativeCalls_RococoGraphicsILandscapeTesselator(args.ss, &platform);
						AddNativeCalls_RococoIKeyboard(args.ss, &platform.keyboard);
						Entities::AddNativeCalls_RococoEntitiesIParticleSystem(args.ss, &platform);
						Audio::AddNativeCalls_RococoAudioILegacySoundControl(args.ss, &platform);
						Graphics::AddNativeCalls_RococoGraphicsIHQFonts(args.ss, &platform);
						Rococo::AddNativeCalls_RococoIInstallationManager(args.ss, &platform);
						AddNativeCalls_RococoIConfig(args.ss, &platform.config);
						Audio::AddNativeCalls_RococoAudioIAudio(args.ss, &platform.audio);
						Rococo::AddNativeCalls_RococoIArchive(args.ss, &platform);
						Rococo::AddNativeCalls_RococoIWorldBuilder(args.ss, &platform);

						const INamespace& ns = args.ss.AddNativeNamespace("MPlat.OS");
						args.ss.AddNativeCall(ns, NativeEnumerateFiles, &platform, "EnumerateFiles (Sys.Type.IString filter)(MPlat.OnFileName callback)->");
					}

					onScriptEvent.OnEvent(args);
				}

				ScriptContext(Platform& _platform, IEventCallback<ScriptCompileArgs>& _onScriptEvent, IEventCallback<cstr>* _onScriptCrash) :
					platform(_platform), onScriptEvent(_onScriptEvent), onScriptCrash(_onScriptCrash) {}

				void Execute(cstr name, ScriptPerformanceStats& stats, bool trace, int32 id)
				{
					try
					{
						IDE::ExecuteSexyScriptLoop(stats,
							1024_kilobytes, 
							platform.ssFactory,
							platform.sourceCache,
							platform.debuggerWindow,
							name, 
							id,
							(int32)128_kilobytes, 
							*this, 
							*this, 
							platform.appControl,
							trace);
					}
					catch (IException&)
					{
						if (shutdownOnFail) platform.appControl.ShutdownApp();
						throw;
					}
				}

				bool addPlatform;
			} sc(platform, _onScriptEvent, onScriptCrash);

			sc.addPlatform = addPlatform;
			sc.shutdownOnFail = shutdownOnFail;

			sc.Execute(name, stats, trace, id);
		}

		void RunBareScript(
			ScriptPerformanceStats& stats,
			IEventCallback<ScriptCompileArgs>& _onScriptEvent, 
			const char* name,
			int id,
			IScriptSystemFactory& ssf,
			IDebuggerWindow& debugger,
			ISourceCache& sources,
			OS::IAppControl& appControl
		)
		{
			struct ScriptContext : public IEventCallback<ScriptCompileArgs>, public IDE::IScriptExceptionHandler
			{
				IEventCallback<ScriptCompileArgs>& onScriptEvent;
				bool shutdownOnFail;

				void Free() override
				{

				}

				IDE::EScriptExceptionFlow GetScriptExceptionFlow(cstr source, cstr message) override
				{
					return IDE::EScriptExceptionFlow_Retry;
				}

				void OnEvent(ScriptCompileArgs& args) override
				{
					onScriptEvent.OnEvent(args);
				}

				ScriptContext(IEventCallback<ScriptCompileArgs>& _onScriptEvent) :
					onScriptEvent(_onScriptEvent) {}

				void Execute(cstr name,
					ScriptPerformanceStats stats,
					int32 id,
					IScriptSystemFactory& ssf, 
					IDebuggerWindow& debugger, 
					ISourceCache& sources,
					OS::IAppControl& appControl)
				{
					try
					{
						ScriptPerformanceStats stats;
						IDE::ExecuteSexyScriptLoop(stats,
							1024_kilobytes,
							ssf,
							sources,
							debugger,
							name,
							id,
							(int32)128_kilobytes,
							*this,
							*this,
							appControl,
							false);
					}
					catch (IException&)
					{
						throw;
					}
				}

			} sc(_onScriptEvent);

			sc.Execute(name, stats, id, ssf, debugger, sources, appControl);
		}

		void RunMPlatConfigScript(OUT IConfig& config,
			Script::IScriptSystemFactory& ssf,
			IDebuggerWindow& debugger,
			ISourceCache& sources,
			OS::IAppControl& appControl
		)
		{
			struct : IEventCallback<ScriptCompileArgs>
			{
				IConfig* config;

				void OnEvent(ScriptCompileArgs& args) override
				{
					AddNativeCalls_RococoIConfig(args.ss, config);
				}
			} onCompile;
			onCompile.config = &config;

			ScriptPerformanceStats stats = { 0 };
			RunBareScript(
				stats, onCompile, "!scripts/config_mplat.sxy", 0,
				ssf, debugger, sources, appControl
			);
		}
	} // M
} // Rococo