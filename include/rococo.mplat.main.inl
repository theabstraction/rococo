#pragma once

// Include this header in your executable once

#include <rococo.api.h>
#include <rococo.renderer.h>
#include <rococo.os.win32.h>
#include <rococo.mplat.h>
#include <rococo.libs.inl>
#include <sexy.lib.s-parser.h>
#include <sexy.lib.util.h>
#include <sexy.lib.script.h>
#include <sexy.lib.sexy-util.h>

#ifdef _WIN32
# ifdef _DEBUG
#  pragma comment(lib, "rococo.mplat.debug.lib")
#  pragma comment(lib, "rococo.file.browser.debug.lib")
#  pragma comment(lib, "dx11.renderer.debug.lib")
# else
#  pragma comment(lib, "rococo.file.browser.lib")
#  pragma comment(lib, "rococo.mplat.lib")
#  pragma comment(lib, "dx11.renderer.lib")
# endif
#endif

#pragma comment(lib, "xinput.lib")

#include "..\dx11.renderer\dx11.imports.inl"

namespace Rococo
{
   void UniqueMPlatFunction() {} // If this causes you grief, you included the header twice in the same app. Naughty!

   /*  ...implement WinMain and call M_Platorm_Win64_Main :
   E.g:
   int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
   {
      struct : IAppFactory
      {
         IApp* CreateApp(Platform& platform) override
         {
            return new TestApp(platform);
         }
      } factory;

      return M_Platorm_Win64_Main(hInstance, factory, "M-Platform Test App");
   }
   */
   int M_Platorm_Win64_Main(HINSTANCE hInstance, IAppFactory& factory, cstr title, HICON hLarge, HICON hSmall);
   int M_Platorm_Win64_MainDirect(HINSTANCE hInstance, IDirectAppFactory& factory, cstr title, HICON hLarge, HICON hSmall);
}
