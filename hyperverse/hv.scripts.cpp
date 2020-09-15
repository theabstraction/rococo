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

	HV::ISectorBuilder* FactoryConstructHVSectorBuilder(HV::ISectors* _context)
	{
		return _context->Builder();
	}

	HV::ISectorWallTesselator* FactoryConstructHVSectorWallTesselator(HV::ISectorWallTesselator* _context)
	{
		return _context;
	}

	HV::ISectorFloorTesselator* FactoryConstructHVSectorFloorTesselator(HV::ISectorFloorTesselator* _context)
	{
		return _context;
	}

	HV::ISectorComponents* FactoryConstructHVSectorComponents(HV::ISectorComponents* _context)
	{
		return _context;
	}

	HV::ISectorEnumerator* FactoryConstructHVSectorEnumerator(HV::ISectorEnumerator* _context)
	{
		return _context;
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
   void RunEnvironmentScript(Cosmos& e, cstr name, bool releaseAfterUse, bool trace)
   {
      class ScriptContext: public IEventCallback<ScriptCompileArgs>
      {
         Cosmos& e;

         virtual void OnEvent(ScriptCompileArgs& args)
         { 
#ifdef _DEBUG
            args.ss.AddNativeLibrary("rococo.sexy.mathsex.debug");
#else
            args.ss.AddNativeLibrary("rococo.sexy.mathsex");
#endif
            AddNativeCalls_HVIPlayer(args.ss, &e.players);
			AddNativeCalls_HVISectorBuilder(args.ss, &e.sectors);
			AddNativeCalls_HVISectorAIBuilder(args.ss, nullptr);
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