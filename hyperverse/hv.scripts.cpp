#include "hv.h"

#include "rococo.mplat.h"

#include <sexy.script.h>
#include <sexy.vm.h>
#include <sexy.vm.cpu.h>

namespace // Script factories
{
   using namespace HV;
   using namespace Rococo;

   HV::Entities::IInstances* FactoryConstructHVEntitiesInstances(HV::Entities::IInstances* ins)
   {
      return ins;
   }

   HV::Graphics::ISceneBuilder* FactoryConstructHVGraphicsSceneBuilder(HV::Graphics::ISceneBuilder* sb)
   {
      return sb;
   }

   HV::Graphics::ICamera* FactoryConstructHVGraphicsCamera(HV::Graphics::ICamera* camera)
   {
      return camera;
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

   HV::Entities::IMobiles* FactoryConstructHVEntitiesMobiles(HV::Entities::IMobiles* mobs)
   {
      return mobs;
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
            Entities::AddNativeCalls_HVEntitiesIInstances(args.ss, &e.instances);
            Entities::AddNativeCalls_HVEntitiesIMobiles(args.ss, &e.mobiles);
            Graphics::AddNativeCalls_HVGraphicsISceneBuilder(args.ss, &e.scene.Builder());
            Graphics::AddNativeCalls_HVGraphicsICamera(args.ss, &e.camera);   
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