#include "rococo.dx11.api.h"
#include "dx11helpers.inl"

namespace Rococo
{
   namespace DX11
   {
      D3D11_TEXTURE2D_DESC GetDepthDescription(HWND hWnd)
      {
         RECT rect;
         GetClientRect(hWnd, &rect);

         D3D11_TEXTURE2D_DESC depthStencilDesc;
         depthStencilDesc.Width = rect.right - rect.left;
         depthStencilDesc.Height = rect.bottom - rect.top;
         depthStencilDesc.MipLevels = 1;
         depthStencilDesc.ArraySize = 1;
         depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
         depthStencilDesc.SampleDesc.Count = 1;
         depthStencilDesc.SampleDesc.Quality = 0;
         depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
         depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
         depthStencilDesc.CPUAccessFlags = 0;
         depthStencilDesc.MiscFlags = 0;
         return depthStencilDesc;
      }

      ID3D11DepthStencilState* CreateObjectDepthStencilState(ID3D11Device& device)
      {
         D3D11_DEPTH_STENCIL_DESC desc;
         ZeroMemory(&desc, sizeof(desc));
         desc.DepthEnable = TRUE;
         desc.DepthFunc = D3D11_COMPARISON_LESS;
         desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;

         ID3D11DepthStencilState* dss = nullptr;
         VALIDATEDX11(device.CreateDepthStencilState(&desc, &dss));
         return dss;
      }

      ID3D11DepthStencilState* CreateGuiDepthStencilState(ID3D11Device& device)
      {
         D3D11_DEPTH_STENCIL_DESC desc;
         ZeroMemory(&desc, sizeof(desc));
         ID3D11DepthStencilState* dss = nullptr;
         VALIDATEDX11(device.CreateDepthStencilState(&desc, &dss));
         return dss;
      }
   } // DX11
} // Rococo
