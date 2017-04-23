#include "hv.defaults.h"
#include "hv.events.h"

#include <rococo.os.win32.h>

#include <rococo.libs.inl>
#include <rococo.window.h>

#include <rococo.strings.h>
#include <rococo.renderer.h>
#include <rococo.dx11.renderer.win32.h>
#include <rococo.window.h>
#include <rococo.strings.h>

#include <vector>
#include <algorithm>

#include <sexy.script.h>
#include <rococo.sexy.ide.h>

using namespace Rococo;
using namespace Rococo::Windows;
using namespace HV::Entities;

namespace Rococo
{
   IOSSupervisor* GetOS();
}

namespace HV
{
   bool QueryYesNo(IWindow& ownerWindow, cstr message)
   {
      rchar title[256];
      GetWindowTextA(ownerWindow, title, 256);
      return ShowMessageBox(Windows::NullParent(), message, title, MB_ICONQUESTION | MB_YESNO) == IDYES;
   }
}

void Main(HANDLE hInstanceLock)
{
   using namespace HV;
   using namespace HV::Events;
   using namespace HV::Graphics;
   using namespace HV::Defaults;

   AutoFree<IOSSupervisor> os = GetOS();
   AutoFree<IInstallationSupervisor> installation = CreateInstallation("content.indicator.txt", *os);
   AutoFree<Rococo::Events::IPublisherSupervisor> publisher(Rococo::Events::CreatePublisher());
   os->Monitor(installation->Content());

   AutoFree<IDX11Window> mainWindow(CreateDX11Window(*installation));
   AutoFree<ISourceCache> sourceCache(CreateSourceCache(*installation));
   AutoFree<IDebuggerWindow> debuggerWindow(Rococo::IDE::CreateDebuggerWindow(mainWindow->Window()));

   AutoFree<IConfigSupervisor> config(CreateConfig());

   rchar srcpath[_MAX_PATH];
   SecureFormat(srcpath, "%sscripts\\native\\", installation->Content());

   Sexy::Script::SetDefaultNativeSourcePath(srcpath);

   AutoFree<IMeshBuilderSupervisor> meshes = CreateMeshBuilder(mainWindow->Renderer());
   AutoFree<IInstancesSupervisor> instances = CreateInstanceBuilder(*meshes, mainWindow->Renderer(), *publisher);
   AutoFree<IMobilesSupervisor> mobiles = CreateMobilesSupervisor(*instances, *publisher);
   AutoFree<ICameraSupervisor> camera = CreateCamera(*instances, *mobiles, mainWindow->Renderer(), *publisher);
   AutoFree<ISceneSupervisor> scene = CreateScene(*instances, *camera, *publisher);
   AutoFree<IPlayerSupervisor> players = CreatePlayerSupervisor(*publisher);
   AutoFree<IKeyboardSupervisor> keyboardSupervisor = CreateKeyboardSupervisor();
   AutoFree<IMouse> mouse = CreateMouse(*publisher);
   AutoFree<IMathsVisitorSupervisor> mathsVisitor = CreateMathsVisitor();
   AutoFree<ISpriteSupervisor> sprites = CreateSpriteSupervisor(mainWindow->Renderer());
   AutoFree<IEditor> editor = CreateEditor(*publisher, *instances, mainWindow->Renderer(), mainWindow->Window());

   HV::Cosmos e
   {
      *config,
      *publisher,
      *installation,
      *sourceCache,
      *debuggerWindow,
      mainWindow->Window(), 
      mainWindow->Renderer(),
      *scene,
      *meshes,
      *instances,
      *mobiles,
      *camera,
      *sprites,
      *players,
      *keyboardSupervisor,
      *mouse,
      *mathsVisitor,
      *editor
   };

   SetDefaults(*config);

 
   RunEnvironmentScript(e, "!scripts/hv/config.sxy");

   SetWindowTextA(mainWindow->Window(), config->GetText(HV::Defaults::appTitle.key));

   RunEnvironmentScript(e, "!scripts/hv/keys.sxy");
   RunEnvironmentScript(e, "!scripts/hv/controls.sxy");
   RunEnvironmentScript(e, "!scripts/hv/main.sxy");

   e.renderer.AddOverlay(1000, &mathsVisitor->Overlay());
   e.renderer.AddOverlay(1001, &e.editor.Overlay());

   AutoFree<IApp> app = CreateHVApp(e);

   mainWindow->Run(hInstanceLock, *app);
}

int CALLBACK WinMain(HINSTANCE _hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
   using namespace Rococo;

   Sexy::OS::SetBreakPoints(Sexy::OS::BreakFlag_All);

   HANDLE hInstanceLock = CreateEventA(nullptr, TRUE, FALSE, "HV_InstanceLock");

   if (GetLastError() == ERROR_ALREADY_EXISTS)
   {
      SetEvent(hInstanceLock);

      if (IsDebuggerPresent())
      {
         ShowMessageBox(Windows::NoParent(), "Hyperverse is already running", "Hyperverse", MB_ICONEXCLAMATION);
      }
      return -1;
   }

   try
   {
      InitRococoWindows(_hInstance, nullptr, nullptr, nullptr, nullptr);
      Main(hInstanceLock);
   }
   catch (IException& ex)
   {
      ShowErrorBox(NoParent(), ex, "Hyperverse - Fatal Error");
   }

   CloseHandle(hInstanceLock);

   return 0;
}