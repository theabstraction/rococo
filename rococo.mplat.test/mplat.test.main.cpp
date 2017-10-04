#include <rococo.mplat.dynamic.inl>

using namespace Rococo;

namespace Test
{
   IApp* CreateApp(Platform& platform);
}

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
   struct : IAppFactory
   {
      IApp* CreateApp(Platform& platform) override
      {
         return Test::CreateApp(platform);
      }
   } factory;

   return LoadPlatformDll_AndRun(hInstance, factory, "M-Platform Test App", MPLAT_RELEASE, nullptr, nullptr);
}