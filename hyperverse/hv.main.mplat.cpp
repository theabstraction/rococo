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

   InitRococoWindows(hInstance, nullptr, nullptr, nullptr, nullptr);
   return LoadPlatformDll_AndInit(hInstance, factory, "Hyperverse", MPLAT_DEBUG, nullptr, nullptr);
}