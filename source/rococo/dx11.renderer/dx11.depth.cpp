#include "dx11.renderer.h"
#include "dx11helpers.inl"
#include <rococo.os.h>
#include <rococo.time.h>

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
         desc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
         desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;

         ID3D11DepthStencilState* dss = nullptr;
         VALIDATEDX11(device.CreateDepthStencilState(&desc, &dss));
         return dss;
      }

	  ID3D11DepthStencilState* CreateObjectDepthStencilState_NoWrite(ID3D11Device& device)
	  {
		  D3D11_DEPTH_STENCIL_DESC desc;
		  ZeroMemory(&desc, sizeof(desc));
		  desc.DepthEnable = TRUE;
		  desc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
		  desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
		  
		  ID3D11DepthStencilState* dss = nullptr;
		  VALIDATEDX11(device.CreateDepthStencilState(&desc, &dss));
		  return dss;
	  }

	  ID3D11DepthStencilState* CreateNoDepthCheckOrWrite(ID3D11Device& device)
	  {
		  D3D11_DEPTH_STENCIL_DESC desc;
		  ZeroMemory(&desc, sizeof(desc));
		  desc.DepthEnable = FALSE;
		  desc.DepthFunc = D3D11_COMPARISON_ALWAYS;
		  desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;

		  ID3D11DepthStencilState* dss = nullptr;
		  VALIDATEDX11(device.CreateDepthStencilState(&desc, &dss));
		  return dss;
	  }

      ID3D11DepthStencilState* CreateGuiDepthStencilState(ID3D11Device& device)
      {
         D3D11_DEPTH_STENCIL_DESC desc;
         ZeroMemory(&desc, sizeof(desc));
		 desc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
         ID3D11DepthStencilState* dss = nullptr;
         VALIDATEDX11(device.CreateDepthStencilState(&desc, &dss));
         return dss;
      }

	  bool PrepareDepthRenderFromLight(const Light& light, DepthRenderData& drd)
	  {
		  if (!TryNormalize(light.direction, drd.direction))
		  {
			  return false;
		  }

		  drd.direction.w = 0;
		  drd.eye = Vec4::FromVec3(light.position, 1.0f);
		  drd.fov = light.fov;

		  Matrix4x4 directionToCameraRot = RotateDirectionToNegZ(drd.direction);

		  Matrix4x4 cameraToDirectionRot = TransposeMatrix(directionToCameraRot);
		  drd.right = cameraToDirectionRot * Vec4{ 1, 0, 0, 0 };
		  drd.up = cameraToDirectionRot * Vec4{ 0, 1, 0, 0 };

		  drd.worldToCamera = directionToCameraRot * Matrix4x4::Translate(-drd.eye);

		  drd.nearPlane = light.nearPlane;
		  drd.farPlane = light.farPlane;

		  Matrix4x4 cameraToScreen = Matrix4x4::GetRHProjectionMatrix(drd.fov, 1.0f, drd.nearPlane, drd.farPlane);

		  drd.worldToScreen = cameraToScreen * drd.worldToCamera;

		  Time::ticks t = Time::TickCount();
		  Time::ticks ticksPerSecond = Time::TickHz();

		  Time::ticks oneMinute = ticksPerSecond * 60;

		  Time::ticks secondOfMinute = t % oneMinute;

		  drd.time = Seconds{ (secondOfMinute / (float)ticksPerSecond) * 0.9999f };

		  return true;
	  }
   } // DX11
} // Rococo
