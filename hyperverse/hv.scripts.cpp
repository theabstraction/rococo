#include "hv.h"

#include "rococo.mplat.h"

#include <sexy.script.h>
#include <sexy.vm.h>
#include <sexy.vm.cpu.h>
#include <rococo.sexy.api.h>

namespace // Script factories
{
	using namespace HV;
	using namespace Rococo;

	HV::IPlayer* FactoryConstructHVPlayer(HV::IPlayerSupervisor* players, int32 index)
	{
		return players->GetPlayer(index);
	}

	HV::IScriptConfig* FactoryConstructHVScriptConfig(HV::IScriptConfig* context)
	{
		return context;
	}

	HV::ISectorBuilder* FactoryConstructHVSectorBuilder(HV::ISectors* context)
	{
		return context->Builder();
	}

	HV::ISectorWallTesselator* FactoryConstructHVSectorWallTesselator(HV::ISectorWallTesselator* context)
	{
		return context;
	}

	HV::ISectorFloorTesselator* FactoryConstructHVSectorFloorTesselator(HV::ISectorFloorTesselator* context)
	{
		return context;
	}

	HV::ISectorComponents* FactoryConstructHVSectorComponents(HV::ISectorComponents* context)
	{
		return context;
	}

	HV::ISectorEnumerator* FactoryConstructHVSectorEnumerator(HV::ISectorEnumerator* context)
	{
		return context;
	}

	HV::IObjectPrototypeBuilder* FactoryConstructHVObjectPrototypeBuilder(HV::IObjectPrototypeBuilder* context)
	{
		return context;
	}

	HV::IObjectPrototype* FactoryConstructHVObjectPrototypeBuilder(HV::IObjectPrototype* context)
	{
		return context;
	}
}

using namespace HV;

namespace HV
{
   HV::ICorridor* FactoryConstructHVCorridor(HV::ICorridor* _context);
}

#include "hv.sxh.inl"

namespace HV
{
	void AddMathsEx(IPublicScriptSystem& ss)
	{
#ifdef _DEBUG
		ss.AddNativeLibrary("rococo.sexy.mathsex.debug");
#else
		ss.AddNativeLibrary("rococo.sexy.mathsex");
#endif
	}

	void RunEnvironmentScript(Cosmos& e, cstr name, bool releaseAfterUse, bool trace)
	{
		class ScriptContext : public IEventCallback<ScriptCompileArgs>
		{
			Cosmos& e;

			virtual void OnEvent(ScriptCompileArgs& args)
			{
				AddMathsEx(args.ss);
				AddNativeCalls_HVIPlayer(args.ss, &e.players);
				AddNativeCalls_HVISectorBuilder(args.ss, &e.sectors);
				AddNativeCalls_HVISectorAIBuilder(args.ss, nullptr);
				AddNativeCalls_HVIObjectPrototypeBuilder(args.ss, &e.object_prototypes);
				AddNativeCalls_HVIObjectPrototype(args.ss, nullptr);
			}

		public:
			ScriptContext(Cosmos& _e) : e(_e) {}

			void Execute(cstr name, bool trace)
			{
				e.platform.utilities.RunEnvironmentScript(*this, name, true, true, trace);
			}
		} sc(e);

		sc.Execute(name, trace);

		if (releaseAfterUse)
		{
			e.platform.sourceCache.Release(name);
		}
	}
}