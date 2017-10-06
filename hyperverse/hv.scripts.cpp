#include "hv.h"

#include "rococo.mplat.h"

#include <sexy.script.h>
#include <sexy.vm.h>
#include <sexy.vm.cpu.h>

namespace // Script factories
{
   using namespace HV;
   using namespace Rococo;

   HV::IPlayer* FactoryConstructHVPlayer(HV::IPlayerSupervisor* players, int32 index)
   {
      return players->GetPlayer(index);
   }

   HV::IConfig* FactoryConstructHVConfig(HV::IConfig* config)
   {
      return config;
   }
}

#include "hv.sxh.inl"

namespace HV
{
   void RunEnvironmentScript(Cosmos& e, cstr name)
   {
      class ScriptContext: public IEventCallback<ScriptCompileArgs>
      {
         Cosmos& e;

         virtual void OnEvent(ScriptCompileArgs& args)
         { 
#ifdef _DEBUG
            args.ss.AddNativeLibrary(SEXTEXT("rococo.sexy.mathsex.debug"));
#else
            args.ss.AddNativeLibrary(SEXTEXT("rococo.sexy.mathsex"));
#endif
            AddNativeCalls_HVIPlayer(args.ss, &e.players);
            AddNativeCalls_HVIConfig(args.ss, &e.config);   
         }

      public:
         ScriptContext(Cosmos& _e) : e(_e) {}

         void Execute(cstr name)
         {
            e.platform.utilities.RunEnvironmentScript(e.platform, *this, name, true);
         }
      } sc(e);

      sc.Execute(name);
   }
}