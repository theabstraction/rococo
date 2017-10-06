#include "hv.h"

#include "rococo.mplat.h"

#include <sexy.script.h>
#include <sexy.vm.h>
#include <sexy.vm.cpu.h>

namespace // Script factories
{
   using namespace HV;
   using namespace Rococo;

   HV::Graphics::ISceneBuilder* FactoryConstructHVGraphicsSceneBuilder(HV::Graphics::ISceneBuilder* sb)
   {
      return sb;
   }

   HV::ISprites* FactoryConstructHVSprites(HV::ISprites* sprites)
   {
      return sprites;
   }

   HV::IPlayer* FactoryConstructHVGraphicsPlayer(HV::IPlayerSupervisor* players, int32 index)
   {
      return players->GetPlayer(index);
   }

   HV::IKeyboard* FactoryConstructHVKeyboard(HV::IKeyboard* kb)
   {
      return kb;
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
            Graphics::AddNativeCalls_HVGraphicsISceneBuilder(args.ss, &e.scene.Builder());
            AddNativeCalls_HVISprites(args.ss, &e.sprites);
            AddNativeCalls_HVIKeyboard(args.ss, &e.keyboard);
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