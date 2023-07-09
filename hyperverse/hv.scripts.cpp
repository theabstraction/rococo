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

	IScriptEnumerator& HVDefaultIncludes()
	{
		static cstr implicitIncludeArray[] =
		{
			"!scripts/hv_sxh.sxy",
			"!scripts/hv/hv.types.sxy",
			"!scripts/mplat_sxh.sxy",
			"!scripts/mplat_pane_sxh.sxy",
			"!scripts/mplat_types.sxy",
			"!scripts/types.sxy",
			"!scripts/audio_types.sxy"
		};

		struct Defaults : IScriptEnumerator
		{
			size_t Count() const override
			{
				return sizeof implicitIncludeArray / sizeof(char*);
			}

			cstr ResourceName(size_t index) const override
			{
				return implicitIncludeArray[index];
			}
		};
		
		static Defaults implicitIncludes;
		return implicitIncludes;
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
				AddNativeCalls_HVIObjectPrototypeBase(args.ss, nullptr);
			}

		public:
			ScriptContext(Cosmos& _e) : e(_e) {}

			void Execute(cstr name, bool trace)
			{
				e.platform.plumbing.utilities.RunEnvironmentScript(HVDefaultIncludes(), *this, name, true, true, trace);
			}
		} sc(e);

		sc.Execute(name, trace);

		if (releaseAfterUse)
		{
			e.platform.scripts.sourceCache.Release(name);
		}
	}
}