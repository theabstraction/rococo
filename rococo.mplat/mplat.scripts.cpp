#include <rococo.mplat.h>

#include <sexy.script.h>
#include <sexy.vm.cpu.h>

namespace Rococo
{
	namespace Graphics
	{
		IFieldTesselator* CreateFieldTesselator();
	}
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

Rococo::Graphics::IRodTesselator* FactoryConstructRococoGraphicsRodTesselator(Rococo::Platform* platform)
{
	return Rococo::Graphics::CreateRodTesselator(*platform);
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

	int a = 1;
}


namespace Rococo
{
	using namespace Rococo::Windows;

	namespace M
	{
		bool QueryYesNo(IWindow& ownerWindow, cstr message);

		void InitScriptSystem(IInstallation& installation)
		{
			char srcpath[Rococo::IO::MAX_PATHLEN];
			SecureFormat(srcpath, sizeof(srcpath), "%sscripts\\native\\", (cstr)installation.Content());

			Rococo::Script::SetDefaultNativeSourcePath(srcpath);
			Rococo::OS::SetBreakPoints(Rococo::OS::BreakFlag_All);
		}

		void RunEnvironmentScript(ScriptPerformanceStats& stats, Platform& platform, IEventCallback<ScriptCompileArgs>& _onScriptEvent, const char* name, bool addPlatform, bool shutdownOnFail, bool trace)
		{
			struct ScriptContext : public IEventCallback<ScriptCompileArgs>, public IDE::IScriptExceptionHandler
			{
				Platform& platform;
				IEventCallback<ScriptCompileArgs>& onScriptEvent;
				bool shutdownOnFail;

				virtual void Free()
				{

				}

				virtual IDE::EScriptExceptionFlow GetScriptExceptionFlow(cstr source, cstr message)
				{
					platform.installation.OS().FireUnstable();

					/* uncomment if you want a dialog box to prompt continuation
					char msg[1024];
					SafeFormat(msg, sizeof(msg), "Error: Do you wish to debug?\n\t%s\n\t%s", source, message);
					if (QueryYesNo(platform.renderer.Window(), msg))
					{
					   return IDE::EScriptExceptionFlow_Retry;
					}
					else
					{
					   if (shutdownOnFail) OS::ShutdownApp();
					   return IDE::EScriptExceptionFlow_Terminate;
					}

					*/

					return IDE::EScriptExceptionFlow_Retry;
				}

				virtual void OnEvent(ScriptCompileArgs& args)
				{
					if (addPlatform)
					{
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
						AddNativeCalls_RococoIKeyboard(args.ss, &platform.keyboard);
						Entities::AddNativeCalls_RococoEntitiesIParticleSystem(args.ss, &platform);
						Audio::AddNativeCalls_RococoAudioILegacySoundControl(args.ss, &platform);

						const INamespace& ns = args.ss.AddNativeNamespace("MPlat.OS");
						args.ss.AddNativeCall(ns, NativeEnumerateFiles, &platform, "EnumerateFiles (Sys.Type.IString filter)(MPlat.StringToBool callback)->");

						args.ss.AddNativeLibrary("rococo.sexy.mathsex");
					}

					AddNativeCalls_RococoIConfig(args.ss, &platform.config);

					onScriptEvent.OnEvent(args);
				}

				ScriptContext(Platform& _platform, IEventCallback<ScriptCompileArgs>& _onScriptEvent) : platform(_platform), onScriptEvent(_onScriptEvent) {}

				void Execute(cstr name, ScriptPerformanceStats& stats, bool trace)
				{
					try
					{
						IDE::ExecuteSexyScriptLoop(stats, 1024_kilobytes, platform.ssFactory, platform.sourceCache, platform.debuggerWindow, name, 0, (int32)128_kilobytes, *this, *this, platform.appControl, trace);
					}
					catch (IException&)
					{
						if (shutdownOnFail) platform.appControl.ShutdownApp();
						throw;
					}
				}

				bool addPlatform;
			} sc(platform, _onScriptEvent);

			sc.addPlatform = addPlatform;
			sc.shutdownOnFail = shutdownOnFail;

			sc.Execute(name, stats, trace);
		}
	}
}