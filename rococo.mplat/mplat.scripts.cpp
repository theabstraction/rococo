#include <rococo.mplat.h>

#include <sexy.script.h>
#include <sexy.vm.cpu.h>

Rococo::Graphics::ISceneBuilder* FactoryConstructRococoGraphicsSceneBuilder(Rococo::Graphics::ISceneBuilder* _context)
{
   return _context;
}

Rococo::Graphics::ICamera* FactoryConstructRococoGraphicsCamera(Rococo::Graphics::ICamera* _context)
{
   return _context;
}

Rococo::Entities::IMobiles* FactoryConstructRococoEntitiesMobiles(Rococo::Entities::IMobiles* _context)
{
   return _context;
}

Rococo::IPaneBuilder* FactoryConstructRococoPaneBuilder(Rococo::IPaneBuilder* _context)
{
   return _context;
}

Rococo::Entities::IInstances* FactoryConstructRococoEntitiesInstances(Rococo::Entities::IInstances* ins)
{
   return ins;
}

Rococo::Graphics::IMeshBuilder* FactoryConstructRococoGraphicsMeshBuilder(Rococo::Graphics::IMeshBuilder* _context)
{
   return _context;
}

#include <..\rococo.mplat\mplat.sxh.inl>

#include <rococo.sexy.ide.h>
#include <rococo.strings.h>

namespace Rococo
{
   using namespace Rococo::Windows;

   namespace M
   {
      bool QueryYesNo(IWindow& ownerWindow, cstr message);

      void InitScriptSystem(IInstallation& installation)
      {
         rchar srcpath[Rococo::IO::MAX_PATHLEN];
         SecureFormat(srcpath, sizeof(srcpath), "%sscripts\\native\\", installation.Content());

         Rococo::Script::SetDefaultNativeSourcePath(srcpath);
         Rococo::OS::SetBreakPoints(Rococo::OS::BreakFlag_All);
      }

      void RunEnvironmentScript(Platform& platform, IEventCallback<ScriptCompileArgs>& _onScriptEvent, const char* name, bool addPlatform)
      {
         class ScriptContext : public IEventCallback<ScriptCompileArgs>, public IDE::IScriptExceptionHandler
         {
            Platform& platform;
            IEventCallback<ScriptCompileArgs>& onScriptEvent;

            virtual void Free()
            {

            }

            virtual IDE::EScriptExceptionFlow GetScriptExceptionFlow(cstr source, cstr message)
            {
               platform.installation.OS().FireUnstable();

               rchar msg[1024];
               SafeFormat(msg, sizeof(msg), "Error: Do you wish to debug?\n\t%s\n\t%s", source, message);
               if (QueryYesNo(platform.renderer.Window(), msg))
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
               if (addPlatform)
               {
                  Graphics::AddNativeCalls_RococoGraphicsIMeshBuilder(args.ss, &platform.meshes);
                  Entities::AddNativeCalls_RococoEntitiesIInstances(args.ss, &platform.instances);
                  Entities::AddNativeCalls_RococoEntitiesIMobiles(args.ss, &platform.mobiles);
                  Graphics::AddNativeCalls_RococoGraphicsICamera(args.ss, &platform.camera);
                  Graphics::AddNativeCalls_RococoGraphicsISceneBuilder(args.ss, &platform.scene.Builder());
               }
               onScriptEvent.OnEvent(args);
            }

         public:
            ScriptContext(Platform& _platform, IEventCallback<ScriptCompileArgs>& _onScriptEvent) : platform(_platform), onScriptEvent(_onScriptEvent) {}

            void Execute(cstr name)
            {
               try
               {
                  IDE::ExecuteSexyScriptLoop(1024_kilobytes, platform.sourceCache, platform.debuggerWindow, name, 0, (int32)128_kilobytes, *this, *this);
               }
               catch (IException&)
               {
                  Rococo::OS::ShutdownApp();
                  throw;
               }
            }

            bool addPlatform;
         } sc(platform, _onScriptEvent);

         sc.addPlatform = addPlatform;

         sc.Execute(name);
      }
   }
}