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

#include "rococo.renderer.h"

namespace Rococo::DX11
{
	ROCOCOAPI IDX11TextureArray : public Rococo::Textures::ITextureArray
	{
		virtual void Free() = 0;
		virtual void Resize(size_t nElements) = 0;
		virtual int32 Height() const = 0;
		virtual int32 Width() const = 0;
		virtual ID3D11ShaderResourceView* View() = 0;
		virtual DX11::TextureBind& Binding() = 0;
	};

	IDX11TextureArray* CreateDX11TextureArray(ID3D11Device& device, ID3D11DeviceContext& dc);

	ROCOCOAPI IDX11FontRenderer
	{
		virtual void AddTriangle(const GuiVertex triangle[3]) = 0;
		virtual void FlushLayer() = 0;
		virtual bool UseHQFontShaders() = 0;
		virtual bool UseGuiShaders() = 0;
	};

	ROCOCOAPI IDX11HQFontResource : public IHQFontResource
	{
		virtual void Free() = 0;
		virtual const Fonts::ArrayFontMetrics& GetFontMetrics(ID_FONT idFont) = 0;
		virtual void RenderHQText(ID_FONT id, Rococo::Fonts::IHQTextJob& job, IGuiRenderContext::EMode mode) = 0;
	};

	IDX11HQFontResource* CreateDX11HQFonts(IInstallation& installation, IDX11FontRenderer& renderer, ID3D11Device& device, ID3D11DeviceContext& dc);
}

#endif