#include <rococo.dds.win32.h>
#include <rococo.io.h>
#include <rococo.strings.h>

// download from https://github.com/microsoft/DirectXTK and place into /work/DirectXTK/. Select x64 as solution platform
#include "..\..\..\..\DirectXTK\Inc\DDSTextureLoader.h"
#pragma comment(lib, "..\\..\\..\\DirectXTK\\Bin\\Desktop_2026\\x64\\Release\\DirectXTK.lib")

using namespace Rococo;
using namespace Rococo::Graphics;
using namespace Rococo::Strings;

namespace
{
	struct DDSTexture : IDDSTextureSupervisor, IDDSSysTexture
	{
		void Free() override
		{
			if (texture)
			{
				texture->Release();
			}

			if (textureView)
			{
				textureView->Release();
			}
			
			delete this;
		}

		ID3D11Resource& Resource() override
		{
			if (!texture)
			{
				Throw(0, "DDSTexture: No resource");
			}

			return *texture;
		}

		ID3D11ShaderResourceView& TextureView() override
		{
			if (!textureView)
			{
				Throw(0, "DDSTexture: No texture view");
			}

			return *textureView;
		}

		IDDSSysTexture& SysTexture() override
		{
			return *this;
		}

		D3D11_USAGE usage = D3D11_USAGE_DEFAULT;
		unsigned int bindFlags = 0;
		unsigned int cpuAccessFlags = 0;
		unsigned int miscFlags = 0;

	 	ID3D11Resource* texture = nullptr;
		ID3D11ShaderResourceView* textureView = nullptr;
		DirectX::DDS_ALPHA_MODE alphaMode = DirectX::DDS_ALPHA_MODE_UNKNOWN;
	};

	struct DDSSystem : IDDSLoaderSupervisor
	{
		AutoFree<IExpandingBuffer> scratchBuffer;
		IO::IInstallation& installation;
		size_t maxTextureSize;

		DDSSystem(IO::IInstallation& _installation, size_t _maxTxSize) : installation(_installation), maxTextureSize(_maxTxSize)
		{
			scratchBuffer = CreateExpandingBuffer(32_megabytes);
			if (maxTextureSize < 1024 || maxTextureSize > 65536)
			{
				Throw(0, "Unexpected maxTxSize. Expecting 1024 to 65536");
			}
		}

		void Free() override
		{
			delete this;
		}

		IDDSTextureSupervisor* Load(cstr pingPath, Graphics::DDSContext& context) override
		{
			if (!EndsWithI(pingPath, ".dds"))
			{
				Throw(0, "Expecting a .dds file extension");
			}

			scratchBuffer->Resize(0);
			installation.LoadResource(pingPath, *scratchBuffer, 128_megabytes);

			if (scratchBuffer->Length() == 0)
			{
				Throw(0, "%s: zero length", pingPath);
			}

			AutoFree<DDSTexture> tx = new DDSTexture();

			DirectX::DX11::DDS_LOADER_FLAGS loaderFlags = DirectX::DX11::DDS_LOADER_DEFAULT;

			HRESULT hr = DirectX::CreateDDSTextureFromMemoryEx(&context.device, &context.dc, scratchBuffer->GetData(), scratchBuffer->Length(),
				maxTextureSize,
				context.usage,
				context.bindFlags,
				context.cpuAccessFlags,
				context.miscFlags,
				loaderFlags,
				&tx->texture,
				&tx->textureView,
				&tx->alphaMode);

			if (FAILED(hr))
			{
				Throw(hr, "DirectX::CreateDDSTextureFromMemoryEx '%s' failed", pingPath);
			}

			return tx.Detach();
		}
	};
}

namespace Rococo
{
	ROCOCO_DDS_API IDDSLoaderSupervisor* CreateDDSSystem(IO::IInstallation& installation, size_t maxTextureSize)
	{
		return new DDSSystem(installation, maxTextureSize);
	}
}