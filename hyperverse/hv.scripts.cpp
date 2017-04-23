#include "hv.h"

#include <sexy.script.h>
#include <sexy.vm.h>
#include <sexy.vm.cpu.h>

#include <rococo.sexy.ide.h>
#include <rococo.strings.h>

namespace // Script factories
{
   using namespace HV;
   using namespace Rococo;

   HV::Graphics::IMeshBuilder* FactoryConstructHVGraphicsMeshBuilder(HV::Graphics::IMeshBuilder* mb)
   {
      return mb;
   }

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
      class ScriptContext: public IEventCallback<ScriptCompileArgs>, public IDE::IScriptExceptionHandler
      {
         Cosmos& e;

         virtual void Free()
         {

         }

         virtual IDE::EScriptExceptionFlow GetScriptExceptionFlow(cstr source, cstr message)
         {
            e.installation.OS().FireUnstable();

            rchar msg[1024];
            SafeFormat(msg, _TRUNCATE, "Error: Do you wish to debug?\n\t%s\n\t%s", source, message);
            if (HV::QueryYesNo(e.mainWindow, msg))
            {
               return IDE::EScriptExceptionFlow_Retry;
            }
            else
            {
               return IDE::EScriptExceptionFlow_Terminate;
            }
         }

         virtual void OnEvent(ScriptCompileArgs& args)
         { 
#ifdef _DEBUG
            args.ss.AddNativeLibrary(SEXTEXT("rococo.sexy.mathsex.debug"));
#else
            args.ss.AddNativeLibrary(SEXTEXT("rococo.sexy.mathsex"));
#endif
            Entities::AddNativeCalls_HVEntitiesIInstances(args.ss, &e.instances);
            Entities::AddNativeCalls_HVEntitiesIMobiles(args.ss, &e.mobiles);
            Graphics::AddNativeCalls_HVGraphicsIMeshBuilder(args.ss, &e.meshes);
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
            try
            {
               IDE::ExecuteSexyScriptLoop(1024_kilobytes, e.sources, e.debugger, name, 0, (int32)128_kilobytes, *this, *this);
            }
            catch (IException&)
            {
               Rococo::OS::ShutdownApp();
               throw;
            }
         }
      } sc(e);

      sc.Execute(name);
   }
}