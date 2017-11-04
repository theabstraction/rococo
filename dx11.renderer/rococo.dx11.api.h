#ifndef ROCOCO_DX11_API_H
#define ROCOCO_DX11_API_H

#include <rococo.api.h>
#define NOMINMAX
#include <d3d11.h>

namespace Rococo
{
   struct IExpandingBuffer;

   namespace DX11
   {
      D3D11_TEXTURE2D_DESC GetDepthDescription(HWND hWnd);
      DXGI_SWAP_CHAIN_DESC GetSwapChainDescription(HWND hWnd);

      ID3D11DepthStencilState* CreateGuiDepthStencilState(ID3D11Device& device);
      ID3D11DepthStencilState* CreateObjectDepthStencilState(ID3D11Device& device);

      const D3D11_INPUT_ELEMENT_DESC* const GetGuiVertexDesc();
      const uint32 NumberOfGuiVertexElements();

      const D3D11_INPUT_ELEMENT_DESC* const GetObjectVertexDesc();
      const uint32 NumberOfObjectVertexElements();

      ID3D11SamplerState* CreateSpriteSampler(ID3D11Device& device);
      ID3D11SamplerState* CreateObjectSampler(ID3D11Device& device);
	  ID3D11SamplerState* CreateShadowSampler(ID3D11Device& device);
      ID3D11RasterizerState* CreateSpriteRasterizer(ID3D11Device& device);
      ID3D11RasterizerState* CreateObjectRasterizer(ID3D11Device& device);
	  ID3D11RasterizerState* CreateShadowRasterizer(ID3D11Device& device);
      ID3D11BlendState* CreateAlphaBlend(ID3D11Device& device);
      ID3D11BlendState* CreateNoBlend(ID3D11Device& device);
	  ID3D11BlendState* CreateAdditiveBlend(ID3D11Device& device);

      ROCOCOAPI ICountdownConfirmationDialog
      {
         virtual DWORD DoModal(HWND owner /* the owner is greyed out during modal operation */, cstr title, cstr hint, int countdown) = 0;
         virtual void Free() = 0;
      };

      ICountdownConfirmationDialog* CreateCountdownConfirmationDialog();

      struct TextureBind
      {
         ID3D11Texture2D* texture;
         ID3D11ShaderResourceView* view;
      };

      class TextureLoader
      {
         IInstallation& installation;
         ID3D11Device& device;
         ID3D11DeviceContext& dc;
         IExpandingBuffer& scratchBuffer;

      public:
         TextureLoader(IInstallation& installation, ID3D11Device& device, ID3D11DeviceContext& _dc, IExpandingBuffer& _scratchBuffer);
         TextureBind LoadAlphaBitmap(cstr resourceName);
         TextureBind LoadColourBitmap(cstr resourceName);
      };
   } // DX11
} // Rococo

#endif // ROCOCO_DX11_API_H
