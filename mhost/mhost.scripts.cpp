#include "mhost.h"
#include "rococo.mplat.h"
#include <sexy.types.h>
#include <sexy.vm.h>
#include <sexy.vm.cpu.h>
#include <sexy.script.h>

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

	void RunEnvironmentScript(Platform& platform, IEngine* engine, cstr name, bool releaseAfterUse)
	{
		class ScriptContext : public IEventCallback<ScriptCompileArgs>
		{
			Platform& platform;
			IEngine* engine;

			virtual void OnEvent(ScriptCompileArgs& args)
			{
#ifdef _DEBUG
				args.ss.AddNativeLibrary("rococo.sexy.mathsex.debug");
#else
				args.ss.AddNativeLibrary("rococo.sexy.mathsex");
#endif
				AddNativeCalls_MHostIScreenBuilder(args.ss, engine->ScreenBuilder());
				AddNativeCalls_MHostIEngine(args.ss, engine);
				MHost::OS::AddNativeCalls_MHostOS(args.ss);
			}

		public:
			ScriptContext(Platform& _platform, IEngine* _engine) : 
				platform(_platform), engine(_engine) {}

			void Execute(cstr name)
			{
				platform.utilities.RunEnvironmentScript(*this, name, true);
			}
		} sc(platform, engine);

		sc.Execute(name);

		if (releaseAfterUse)
		{
			platform.sourceCache.Release(name);
		}
	}
}