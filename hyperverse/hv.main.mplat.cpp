#include <rococo.mplat.dynamic.inl>
#include "resource.h"

using namespace Rococo;

namespace HV
{
   IApp* CreateApp(Platform& platform);
}

#ifdef _DEBUG
# define MPLAT_LIB MPLAT_DEBUG
#else
# define MPLAT_LIB MPLAT_RELEASE
#endif

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
   struct : IAppFactory
   {
      IApp* CreateApp(Platform& e) override
      {
         return HV::CreateApp(e);
      }
   } factory;

   return LoadPlatformDll_AndRun(hInstance, factory, "Hyperverse", MPLAT_LIB, nullptr, nullptr);
}