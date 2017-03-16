#include "hv.h"
#include <rococo.os.win32.h>

#include <rococo.libs.inl>
#include <rococo.window.h>

#include <rococo.strings.h>
#include <rococo.renderer.h>
#include <rococo.dx11.renderer.win32.h>
#include <rococo.window.h>
#include "rococo.strings.h"

#include <vector>
#include <algorithm>

#ifdef _DEBUG
# pragma comment(lib, "dx11.renderer.debug.lib")
# pragma comment(lib, "rococo.os.win32.debug.lib")
# pragma comment(lib, "rococo.maths.debug.lib")
# pragma comment(lib, "rococo.util.debug.lib")
# pragma comment(lib, "rococo.sexy.ide.debug.lib")
#else
# pragma comment(lib, "dx11.renderer.lib")
# pragma comment(lib, "rococo.os.win32.lib")
# pragma comment(lib, "rococo.maths.lib")
# pragma comment(lib, "rococo.util.lib")
# pragma comment(lib, "rococo.sexy.ide.lib")
#endif

#include <sexy.lib.s-parser.h>
#include <sexy.lib.util.h>
#include <sexy.lib.script.h>

#include <sexy.script.h>
#include <rococo.sexy.ide.h>

using namespace Rococo;
using namespace Rococo::Windows;

namespace Rococo
{
   IOSSupervisor* GetOS();
}

namespace HV
{
   bool QueryYesNo(IWindow& ownerWindow, const wchar_t* message)
   {
      wchar_t title[256];
      GetWindowText(ownerWindow, title, 256);
      return MessageBox(ownerWindow, message, title, MB_ICONQUESTION | MB_YESNO) == IDYES;
   }
}

void Main(HANDLE hInstanceLock)
{
   AutoFree<IOSSupervisor> os = GetOS();
   AutoFree<IInstallationSupervisor> installation = CreateInstallation(L"content.indicator.txt", *os);
   AutoFree<Events::IPublisherSupervisor> publisher(Events::CreatePublisher());
   os->Monitor(installation->Content());

   AutoFree<IDX11Window> mainWindow(CreateDX11Window(*installation));
   AutoFree<ISourceCache> sourceCache(CreateSourceCache(*installation));
   AutoFree<IDebuggerWindow> debuggerWindow(Rococo::IDE::CreateDebuggerWindow(&mainWindow->Window()));

   SetWindowText(mainWindow->Window(), L"Hyperverse");

   wchar_t srcpath[_MAX_PATH];
   SecureFormat(srcpath, L"%sscripts\\native\\", installation->Content());

   Sexy::Script::SetDefaultNativeSourcePath(srcpath);

   AutoFree<HV::Graphics::IMeshBuilderSupervisor> meshes = HV::Graphics::CreateMeshBuilder(mainWindow->Renderer());
   AutoFree<HV::Graphics::IInstancesSupervisor> instances = HV::Graphics::CreateInstanceBuilder(*meshes, mainWindow->Renderer());
   AutoFree<HV::Graphics::ICameraSupervisor> camera = HV::Graphics::CreateCamera(*instances, mainWindow->Renderer());
   AutoFree<HV::Graphics::ISceneSupervisor> scene = HV::Graphics::CreateScene(*instances, *camera);
 

   HV::Cosmos e
   {
      *publisher,
      *installation,
      *sourceCache,
      *debuggerWindow,
      mainWindow->Window(), 
      mainWindow->Renderer(),
      *scene,
      *meshes,
      *instances,
      *camera
   };

   RunEnvironmentScript(e, L"!scripts/hv/main.sxy");

   AutoFree<IApp> app = HV::CreateHVApp(e);

   mainWindow->Run(hInstanceLock, *app);
}

int CALLBACK WinMain(HINSTANCE _hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
   using namespace Rococo;

   Sexy::OS::SetBreakPoints(Sexy::OS::BreakFlag_All);

   HANDLE hInstanceLock = CreateEvent(nullptr, TRUE, FALSE, L"HV_InstanceLock");

   if (GetLastError() == ERROR_ALREADY_EXISTS)
   {
      SetEvent(hInstanceLock);

      if (IsDebuggerPresent())
      {
         MessageBox(nullptr, L"Hyperverse is already running", L"Hyperverse", MB_ICONEXCLAMATION);
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
      ShowErrorBox(NoParent(), ex, L"Hyperverse - Fatal Error");
   }

   CloseHandle(hInstanceLock);

   return 0;
}