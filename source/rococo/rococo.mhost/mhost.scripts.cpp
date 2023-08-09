#include "mhost.h"
#include "rococo.mplat.h"
#include <sexy.types.h>
#include <sexy.vm.h>
#include <sexy.vm.cpu.h>
#include <sexy.script.h>
#include <rococo.strings.h>
#include <rococo.sexy.api.h>
#include <rococo.package.h>
#include <rococo.stl.allocators.h>

using namespace Rococo;

namespace MHost
{
	IEngine* FactoryConstructMHostEngine(IEngine* _context)
	{
		return _context;
	}

	MHost::IDictionaryStream* FactoryConstructMHostDictionaryStream(Rococo::IO::IInstallation* installation)
	{
		return CreateDictionaryStream(*installation);
	}

	namespace OS
	{
		boolean32 IsKeyPressed(const KeyState& keyState, int32 vkCode)
		{
			if (vkCode < 0 || vkCode >= 256)
			{
				return false;
			}
			else
			{
				return (keyState.keys[vkCode] & 0x80) != 0;
			}
		}
	}

	namespace Maths
	{
		float Clamp0to1(float f)
		{
			return f < 0 ? 0 : (f > 1.0f ? 1.0f : f);
		}
	}

	namespace Graphics
	{
		int32 FloatToColourComponent(float f)
		{
			return (int32) (255.0f * Maths::Clamp0to1(f));
		}

		RGBAb ToRGBAb(const RGBA& colour)
		{
			int32 R = FloatToColourComponent(colour.red);
			int32 G = FloatToColourComponent(colour.green);
			int32 B = FloatToColourComponent(colour.blue);
			int32 A = FloatToColourComponent(colour.alpha);
			return RGBAb((uint8) R, (uint8)G, (uint8)B, (uint8)A);
		}
	}
}

using namespace MHost;

#include "code-gen/mhost.sxh.inl"

namespace MHost
{
	using namespace Rococo;

	inline ObjectStub* InterfaceToInstance(InterfacePointer i)
	{
		auto* p = ((uint8*)i) + (*i)->OffsetToInstance;
		auto* obj = (ObjectStub*)p;
		return obj;
	}

	void AddNativeCallSecurityFromMHostPackage(ScriptCompileArgs& args, cstr nativeNamespace)
	{
		Rococo::Script::AddNativeCallSecurity(args.ss, nativeNamespace, "Package[mhost]");
	}

	void AddMHostNativeCallSecurity(ScriptCompileArgs& args)
	{
		Rococo::Script::AddNativeCallSecurity_ToSysNatives(args.ss);
		AddNativeCallSecurityFromMHostPackage(args, "MHost.Native");
		AddNativeCallSecurityFromMHostPackage(args, "MPlat.Native");
		AddNativeCallSecurityFromMHostPackage(args, "Rococo.Native");
		AddNativeCallSecurityFromMHostPackage(args, "Rococo.GUI.Native");
		AddNativeCallSecurityFromMHostPackage(args, "Rococo.ECS.Native");
		AddNativeCallSecurityFromMHostPackage(args, "Rococo.Audio.Native");
		AddNativeCallSecurityFromMHostPackage(args, "Rococo.Configuration.Native");
		AddNativeCallSecurityFromMHostPackage(args, "Rococo.Entities.Native");
		AddNativeCallSecurityFromMHostPackage(args, "Rococo.Graphics.Native");
		AddNativeCallSecurityFromMHostPackage(args, "Rococo.MPlat.Native");
		AddNativeCallSecurityFromMHostPackage(args, "Rococo.Components.Animation.Native");
		AddNativeCallSecurityFromMHostPackage(args, "Rococo.Components.Body.Native");
		AddNativeCallSecurityFromMHostPackage(args, "Rococo.Components.Config.Native");
		AddNativeCallSecurityFromMHostPackage(args, "Rococo.Components.Skeleton.Native");
	}

	void RegisterMHostPackage(ScriptCompileArgs& args, IPackage* package)
	{
		args.ss.RegisterPackage(package);
		args.ss.LoadSubpackages("MHost", "mhost");
		args.ss.AddNativeLibrary("rococo.sexy.mathsex");
	}

	IScriptCompilationEventHandler& GetBaseCompileOptions()
	{
		struct CLOSURE : IScriptCompilationEventHandler
		{
			void OnCompile(ScriptCompileArgs& args) override
			{
				MHost::AddMHostNativeCallSecurity(args);
				args.ss.AddNativeLibrary("rococo.sexy.mathsex");
			} 

			IScriptEnumerator* ImplicitIncludes() override
			{
				return nullptr;
			}
		};

		static CLOSURE closure;
		return closure;
	}

	void RunMHostEnvironmentScript(Platform& platform, IEngineSupervisor* engine, cstr name, bool releaseAfterUse, bool trace, IPackage& package, Strings::IStringPopulator* onScriptCrash, StringBuilder* declarationBuilder)
	{
		class ScriptContext : public IScriptCompilationEventHandler
		{
			Platform& platform;
			IEngineSupervisor* engine;
			IPackage& package;
			const IStructure* exprStruct = nullptr;
			StringBuilder* declarationBuilder = nullptr;

			AutoFree<ISourceCache> privateSourceCache;

			void LoadExpression(NativeCallEnvironment& _nce)
			{
				InterfacePointer pStringInterface;
				ReadInput(0, pStringInterface, _nce);

				auto* stub = (CStringConstant*) MHost::InterfaceToInstance(pStringInterface);
				cstr fileName = stub->pointer;

				auto* tree = privateSourceCache->GetSource(fileName);

				const int reflModule = 3;
				exprStruct = _nce.ss.PublicProgramObject().GetModule(reflModule).FindStructure("Expression");
				if (exprStruct == nullptr) Rococo::Throw(0, "Could not find class [Expression] in module %d (Sys.Reflection.sxy)", reflModule);

				auto* sExpression = _nce.ss.Represent(*exprStruct, &tree->Root());
				InterfacePointer pExpr = &sExpression->header.pVTables[0];
				WriteOutput(0, pExpr, _nce);
			}

			static void NativeLoadExpression(NativeCallEnvironment& _nce)
			{
				((ScriptContext*)(_nce.context))->LoadExpression(_nce);
			}

			IScriptEnumerator* ImplicitIncludes() override
			{
				return nullptr;
			}

			void OnCompile(ScriptCompileArgs& args) override
			{
				args.ss.RegisterPackage(&package);

				GetBaseCompileOptions().OnCompile(args);

				Interop::AddNativeCalls_MHostIGui(args.ss, nullptr);
				Interop::AddNativeCalls_MHostIEngine(args.ss, engine);
				MHost::OS::AddNativeCalls_MHostOS(args.ss);
				MHost::Graphics::AddNativeCalls_MHostGraphics(args.ss);
				Interop::AddNativeCalls_MHostIDictionaryStream(args.ss, &platform.os.installation);

				auto& mhostOS = args.ss.AddNativeNamespace("MHost.OS");
				args.ss.AddNativeCall(mhostOS, ScriptContext::NativeLoadExpression, this, "LoadExpression (Sys.Type.IString name) -> (Sys.Reflection.IExpression s)", __FUNCTION__, __LINE__);

				engine->SetRunningScriptContext(&args.ss);

				engine->OnCompile(args.ss);
			}

		public:
			ScriptContext(Platform& _platform, IEngineSupervisor* _engine, IPackage& _package, StringBuilder* _declarationBuilder) :
				platform(_platform), engine(_engine), package(_package), declarationBuilder(_declarationBuilder)
			{
				privateSourceCache = CreateSourceCache(platform.os.installation, Rococo::Memory::GetSexyAllocator());
			}

			Strings::IStringPopulator* onScriptCrash;

			void Execute(cstr name, bool trace)
			{
				platform.plumbing.utilities.RunEnvironmentScript(Rococo::NoImplicitIncludes(), *this, name, true, true, trace, onScriptCrash, declarationBuilder);
				engine->SetRunningScriptContext(nullptr);
			}
		} sc(platform, engine, package, declarationBuilder);

		sc.onScriptCrash = onScriptCrash;

		sc.Execute(name, trace);

		if (releaseAfterUse)
		{
			platform.scripts.sourceCache.Release(name);
		}
	}

	struct ScriptDispatcher: IScriptDispatcher
	{
		IPublicScriptSystem* ss = nullptr;
		
		struct GuiSexyObject
		{
			ObjectStub header;
			IGui* gui;
		}* guiSexyObject = nullptr;

		void Free() override
		{
			delete this;
		}

		const IStructure* FindStructureInModule(IPublicProgramObject& ppo, cstr moduleName, cstr structName)
		{
			for (int i = 0; i < ppo.ModuleCount(); i++)
			{
				auto& m = ppo.GetModule(i);
				if (EndsWith(m.Name(), moduleName))
				{
					auto* s = m.FindStructure(structName);
					if (s == nullptr) break;
					return s;
				}
			}

			Throw(0, "Could not find %s in the script %s", structName, moduleName);
		}

		void OnCompile(IPublicScriptSystem&)
		{
			guiSexyObject = nullptr;
		}

		void RouteGuiToScript(IPublicScriptSystem* ss, IGui* gui, const GuiPopulator& populator) override
		{
			if (this->ss != ss || guiSexyObject == nullptr)
			{
				this->ss = ss;
				const IStructure* proxyIGuiStruct = FindStructureInModule(ss->PublicProgramObject(), "mhost_sxh.sxy", "ProxyIGui");
				guiSexyObject = (GuiSexyObject*) ss->Represent(*proxyIGuiStruct, gui);
			}

			guiSexyObject->gui = gui;
			auto* pInterface = (InterfacePointer)(((uint8*)guiSexyObject) + ObjectStub::BYTECOUNT_INSTANCE_TO_INTERFACE0);
			ss->DispatchToSexyClosure(pInterface, populator);
			guiSexyObject->gui = nullptr;
		}
	};

	IScriptDispatcher* CreateScriptDispatcher()
	{
		return new ScriptDispatcher();
	}
}