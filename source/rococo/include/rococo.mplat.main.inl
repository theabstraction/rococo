// Copyright (c)2025 Mark Anthony Taylor. Email: mark.anthony.taylor@gmail.com. All rights reserved.
#pragma once

// Include this header in your executable once

#include <rococo.api.h>
#include <rococo.renderer.h>
#include <rococo.os.win32.h>
#include <rococo.mplat.h>

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
