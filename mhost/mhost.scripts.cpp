#include "mhost.h"
#include "rococo.mplat.h"
#include <sexy.types.h>
#include <sexy.vm.h>
#include <sexy.vm.cpu.h>
#include <sexy.script.h>
#include <rococo.strings.h>


using namespace Rococo;

namespace MHost
{
	IEngine* FactoryConstructMHostEngine(IEngine* _context)
	{
		return _context;
	}

	MHost::IDictionaryStream* FactoryConstructMHostDictionaryStream(Rococo::IInstallation* installation)
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
			return RGBAb(R, G, B, A);
		}
	}
}

using namespace MHost;

#include "mhost.sxh.inl"
#include "mhost.inl"

namespace MHost
{
	using namespace Rococo;

	inline ObjectStub* InterfaceToInstance(InterfacePointer i)
	{
		auto* p = ((uint8*)i) + (*i)->OffsetToInstance;
		auto* obj = (ObjectStub*)p;
		return obj;
	}

	void RunEnvironmentScript(Platform& platform, IEngineSupervisor* engine, cstr name, bool releaseAfterUse, bool trace)
	{
		class ScriptContext : public IEventCallback<ScriptCompileArgs>
		{
			Platform& platform;
			IEngineSupervisor* engine;
			const IStructure* exprStruct = nullptr;

			AutoFree<ISourceCache> privateSourceCache;

			void LoadExpression(NativeCallEnvironment& _nce)
			{
				Rococo::uint8* _sf = _nce.cpu.SF();
				ptrdiff_t _offset = 2 * sizeof(size_t);

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

			virtual void OnEvent(ScriptCompileArgs& args)
			{
#ifdef _DEBUG
				args.ss.AddNativeLibrary("rococo.sexy.mathsex.debug");
#else
				args.ss.AddNativeLibrary("rococo.sexy.mathsex");
#endif
				AddNativeCalls_MHostIGui(args.ss, nullptr);
				AddNativeCalls_MHostIEngine(args.ss, engine);
				AddNativeCalls_MHostISceneBuilder(args.ss, engine->Scene());
				MHost::OS::AddNativeCalls_MHostOS(args.ss);
				MHost::Graphics::AddNativeCalls_MHostGraphics(args.ss);
				AddNativeCalls_MHostIDictionaryStream(args.ss, &platform.installation);

				auto& mhostOS = args.ss.AddNativeNamespace("MHost.OS");
				args.ss.AddNativeCall(mhostOS, ScriptContext::NativeLoadExpression, this, "LoadExpression (Sys.Type.IString name) -> (Sys.Reflection.IExpression s)");

				engine->SetRunningScriptContext(&args.ss);

				engine->OnCompile(args.ss);
			}

		public:
			ScriptContext(Platform& _platform, IEngineSupervisor* _engine) :
				platform(_platform), engine(_engine) 
			{
				privateSourceCache = CreateSourceCache(platform.installation);
			}

			void Execute(cstr name, bool trace)
			{
				platform.utilities.RunEnvironmentScript(*this, name, true, true, trace);
				engine->SetRunningScriptContext(nullptr);
			}
		} sc(platform, engine);

		sc.Execute(name, trace);

		if (releaseAfterUse)
		{
			platform.sourceCache.Release(name);
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
			return nullptr;
		}

		void OnCompile(IPublicScriptSystem& ss)
		{
			guiSexyObject = nullptr;
		}

		void RouteGuiToScript(IPublicScriptSystem* ss, IGui* gui, const GuiPopulator& populator) override
		{
			if (this->ss != ss || guiSexyObject == nullptr)
			{
				this->ss = ss;
				const IStructure* proxyIGuiStruct = FindStructureInModule(ss->PublicProgramObject(), "mhost.sxh.sxy", "ProxyIGui");
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