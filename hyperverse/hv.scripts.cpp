#include "hv.h"

#include <sexy.script.h>
#include <sexy.vm.h>
#include <sexy.vm.cpu.h>

#include <rococo.sexy.ide.h>
#include <rococo.strings.h>

namespace
{
   using namespace HV;
   using namespace Rococo;

   HV::Graphics::IMeshBuilder* FactoryConstructHVGraphicsMeshBuilder(HV::Graphics::IMeshBuilder* mb)
   {
      return mb;
   }

   HV::Graphics::IInstances* FactoryConstructHVGraphicsInstances(HV::Graphics::IInstances* ins)
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

   HV::IPlayer* FactoryConstructHVGraphicsPlayer(HV::IPlayerSupervisor* players, int32 index)
   {
      return players->GetPlayer(index);
   }

   HV::IKeyboard* FactoryConstructHVKeyboard(HV::IKeyboard* kb)
   {
      return kb;
   }
}

#include "hv.sxh.inl"

namespace HV
{
   void RunEnvironmentScript(Cosmos& e, const wchar_t* name)
   {
      class ScriptContext: public IEventCallback<ScriptCompileArgs>, public IDE::IScriptExceptionHandler
      {
         Cosmos& e;

         virtual void Free()
         {

         }

         virtual IDE::EScriptExceptionFlow GetScriptExceptionFlow(const wchar_t* source, const wchar_t* message)
         {
            e.installation.OS().FireUnstable();

            wchar_t msg[1024];
            SafeFormat(msg, _TRUNCATE, L"Error: Do you wish to debug?\n\t%s\n\t%s", source, message);
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
            Graphics::AddNativeCalls_HVGraphicsIMeshBuilder(args.ss, &e.meshes);
            Graphics::AddNativeCalls_HVGraphicsIInstances(args.ss, &e.instances);
            Graphics::AddNativeCalls_HVGraphicsISceneBuilder(args.ss, &e.scene.Builder());
            Graphics::AddNativeCalls_HVGraphicsICamera(args.ss, &e.camera);
            AddNativeCalls_HVIKeyboard(args.ss, &e.keyboard);
            AddNativeCalls_HVIPlayer(args.ss, &e.players);
         }

      public:
         ScriptContext(Cosmos& _e) : e(_e) {}

         void Execute(const wchar_t* name)
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