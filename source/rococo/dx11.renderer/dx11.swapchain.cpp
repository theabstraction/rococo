#include "dx11.renderer.h"
#include <dxgi1_2.h>

namespace Rococo
{
   namespace DX11
   {
      DXGI_SWAP_CHAIN_DESC MakeSwapChainDescription(HWND hWnd)
      {
         RECT rect;
         GetClientRect(hWnd, &rect);

         DXGI_SWAP_CHAIN_DESC sd;
         ZeroMemory(&sd, sizeof(sd));
         sd.BufferCount = 2;
         sd.BufferDesc.Width = rect.right - rect.left;
         sd.BufferDesc.Height = rect.bottom - rect.top;
         sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
         sd.BufferDesc.RefreshRate.Numerator = 60;
         sd.BufferDesc.RefreshRate.Denominator = 1;
         sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
         sd.OutputWindow = hWnd;
         sd.SampleDesc.Count = 1;
         sd.SampleDesc.Quality = 0;
         sd.Windowed = TRUE;
         sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
         sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
         return sd;
      }
   }
}
