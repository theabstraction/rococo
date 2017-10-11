#include <rococo.mplat.dynamic.inl>

using namespace Rococo;

namespace HV
{
   IApp* CreateApp(Platform& platform);
}

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
   struct : IAppFactory
   {
      IApp* CreateApp(Platform& e) override
      {
         return HV::CreateApp(e);
      }
   } factory;

   cstr lib = IsDebuggerPresent() ? MPLAT_DEBUG : MPLAT_RELEASE;

   return LoadPlatformDll_AndRun(hInstance, factory, "Hyperverse", lib, nullptr, nullptr);
}