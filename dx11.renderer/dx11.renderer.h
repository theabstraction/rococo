#ifndef Rococo_WINDOWS_TEST_H
#define Rococo_WINDOWS_TEST_H

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#include <Rococo.target.h>

#define NOMINMAX
#include <Windows.h>

#include <rococo.api.h>
#include <Rococo.window.h>
#include <rococo.strings.h>
#include <rococo.textures.h>

#include <d3d11.h>

#include "rococo.dx11.api.h"

namespace Rococo::Textures
{
	ROCOCOAPI IDX11TextureArray : public ITextureArray
	{
		virtual void Free() = 0;
		virtual void Resize(size_t nElements) = 0;
		virtual int32 Height() const = 0;
		virtual int32 Width() const = 0;
		virtual ID3D11ShaderResourceView* View() = 0;
		virtual DX11::TextureBind& Binding() = 0;
	};

	IDX11TextureArray* CreateDX11TextureArray(ID3D11Device& device, ID3D11DeviceContext& dc);
}

#endif