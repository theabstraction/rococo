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
}

#include "mhost.sxh.inl"
#include "mhost.os.inl"

namespace MHost
{
	using namespace Rococo;

	void RunEnvironmentScript(Platform& platform, IEngineSupervisor* engine, cstr name, bool releaseAfterUse)
	{
		class ScriptContext : public IEventCallback<ScriptCompileArgs>
		{
			Platform& platform;
			IEngineSupervisor* engine;

			virtual void OnEvent(ScriptCompileArgs& args)
			{
#ifdef _DEBUG
				args.ss.AddNativeLibrary("rococo.sexy.mathsex.debug");
#else
				args.ss.AddNativeLibrary("rococo.sexy.mathsex");
#endif
				AddNativeCalls_MHostIGui(args.ss, nullptr);
				AddNativeCalls_MHostIEngine(args.ss, engine);
				MHost::OS::AddNativeCalls_MHostOS(args.ss);

				engine->SetRunningScriptContext(&args.ss);

				engine->OnCompile(args.ss);
			}

		public:
			ScriptContext(Platform& _platform, IEngineSupervisor* _engine) :
				platform(_platform), engine(_engine) {}

			void Execute(cstr name)
			{
				platform.utilities.RunEnvironmentScript(*this, name, true);
				engine->SetRunningScriptContext(nullptr);
			}
		} sc(platform, engine);

		sc.Execute(name);

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

			Throw(0, "Could not find %s in the script %s", moduleName, structName);
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