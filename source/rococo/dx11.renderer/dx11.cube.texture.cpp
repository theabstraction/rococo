#include "dx11.renderer.h"
#include <rococo.renderer.h>
#include "rococo.dx11.api.h"
#include "rococo.textures.h"
#include "dx11helpers.inl"
#include "rococo.imaging.h"

#include <vector>
#include <rococo.hashtable.h>

using namespace Rococo;
using namespace Rococo::DX11;
using namespace Rococo::Graphics::Textures;

namespace Rococo::DX11
{	
	struct CubeLoader : public DX11::IColourBitmapLoadEvent
	{
		size_t face;
		int width;
		std::vector<RGBAb> expandedBuffer;

		void OnLoad(const RGBAb* pixels, const Vec2i& span) override
		{
			if (width == 0)
			{
				width = span.x;
				if (span.x != span.y)
				{
					Throw(0, "CubeLoader::OnLoad: image was not sqaure");
				}

				expandedBuffer.resize(6 * width * width);
			}

			if (span.x != width || span.y != width)
			{
				Throw(0, "Image span %d x %d did not match the cube texture face width [%d]", span.x, span.y, width);
			}

			size_t sizeOfImageBytes = width * width * sizeof(RGBAb);
			char* dest = (char*)expandedBuffer.data();
			memcpy(dest + face * sizeOfImageBytes, pixels, sizeOfImageBytes);
		}
	};

	struct CubeTextures: public IDX11CubeTextures
	{
		std::vector<DX11::TextureBind> cubeTextureArray;
		stringmap<ID_CUBE_TEXTURE> nameToCubeTexture;

		ID3D11Device& device;
		ID3D11DeviceContext& dc;

		enum { CUBE_ID_BASE = 100000000 };

		int32 cubeMaterialId[6] = { -1,-1,-1,-1,-1,-1 };

		AutoRelease<ID3D11Texture2D> cubeTexture;
		AutoRelease<ID3D11ShaderResourceView> cubeTextureView;

		CubeTextures(ID3D11Device& _device, ID3D11DeviceContext& _dc) : device(_device), dc(_dc)
		{

		}

		ID3D11ShaderResourceView* ShaderResourceView() override
		{
			return cubeTextureView;
		}

		virtual ~CubeTextures()
		{
			for (auto& t : cubeTextureArray)
			{
				t.shaderView->Release();
				t.texture->Release();
			}
		}

		ID_CUBE_TEXTURE CreateCubeTexture(IDX11TextureLoader& textureLoader, cstr path, cstr extension) override
		{
			auto i = nameToCubeTexture.find(path);
			if (i != nameToCubeTexture.end())
			{
				return i->second;
			}

			const char* short_filenames[6] = { "posx", "negx", "posy", "negy", "posz", "negz" };

			CubeLoader cubeloader;
			cubeloader.width = 0;
			cubeloader.face = 0;

			D3D11_SUBRESOURCE_DATA initData[6];

			for (auto f : short_filenames)
			{
				U8FilePath fullpath;
				Format(fullpath, "%s%s.%s", path, f, extension);
				textureLoader.LoadColourBitmapIntoAddress(fullpath, cubeloader);

				size_t sizeofImage = cubeloader.width * cubeloader.width;

				initData[cubeloader.face].pSysMem = cubeloader.expandedBuffer.data() + sizeofImage * cubeloader.face;
				initData[cubeloader.face].SysMemPitch = cubeloader.width * sizeof(RGBAb);
				initData[cubeloader.face].SysMemSlicePitch = 0;

				cubeloader.face++;
			}

			D3D11_TEXTURE2D_DESC desc;
			ZeroMemory(&desc, sizeof(desc));
			desc.Width = cubeloader.width;
			desc.Height = cubeloader.width;
			desc.ArraySize = 6;
			desc.SampleDesc.Count = 1;
			desc.SampleDesc.Quality = 0;
			desc.MipLevels = 1;
			desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			desc.Usage = D3D11_USAGE_DEFAULT;
			desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
			desc.CPUAccessFlags = 0;
			desc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

			ID3D11Texture2D* pTexCube = nullptr;

			VALIDATEDX11(device.CreateTexture2D(&desc, &initData[0], &pTexCube));

			ID3D11ShaderResourceView* view = nullptr;
			HRESULT hr = device.CreateShaderResourceView(pTexCube, nullptr, &view);
			if FAILED(hr)
			{
				pTexCube->Release();
				Throw(hr, "DX11Renderer::CreateCubeTexture: Error creating shader resource view for cube texture");
			}

			DX11::TextureBind tb;
			tb.shaderView = view;
			tb.texture = pTexCube;

			cubeTextureArray.push_back(tb);

			size_t index = cubeTextureArray.size() + CUBE_ID_BASE;
			auto id = ID_CUBE_TEXTURE{ index };

			nameToCubeTexture[path] = id;

			return id;
		}
		void Free() override
		{
			delete this;
		}

		ID3D11ShaderResourceView* GetShaderView(ID_CUBE_TEXTURE id) override
		{
			size_t index = id.value - CUBE_ID_BASE - 1;
			if (index < cubeTextureArray.size())
			{
				return cubeTextureArray[index].shaderView;
			}
			else
			{
				return nullptr;
			}
		}

		void SyncCubeTexture(int32 XMaxFace, int32 XMinFace, int32 YMaxFace, int32 YMinFace, int32 ZMaxFace, int32 ZMinFace, IDX11TextureArray& materialArray) override
		{
			int32 newMaterialids[6] = { XMaxFace, XMinFace, YMaxFace, YMinFace, ZMaxFace, ZMinFace };

			if (cubeTexture)
			{
				D3D11_TEXTURE2D_DESC desc;
				cubeTexture->GetDesc(&desc);

				if (desc.Width != (uint32) materialArray.Width())
				{
					cubeTexture = nullptr;
					cubeTextureView = nullptr;
				}
			}

			if (!cubeTexture)
			{
				D3D11_TEXTURE2D_DESC desc;
				ZeroMemory(&desc, sizeof(desc));
				desc.Width = materialArray.Width();
				desc.Height = materialArray.Width();
				desc.ArraySize = 6;
				desc.SampleDesc.Count = 1;
				desc.SampleDesc.Quality = 0;
				desc.MipLevels = 1;
				desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
				desc.Usage = D3D11_USAGE_DEFAULT;
				desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
				desc.CPUAccessFlags = 0;
				desc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

				ID3D11Texture2D* pTexCube = nullptr;
				VALIDATEDX11(device.CreateTexture2D(&desc, nullptr, &pTexCube));
				cubeTexture = pTexCube;

				ID3D11ShaderResourceView* view = nullptr;
				HRESULT hr = device.CreateShaderResourceView(pTexCube, nullptr, &view);
				if FAILED(hr)
				{
					pTexCube->Release();
					Throw(hr, "DX11Renderer::SyncCubeTexture: Error creating shader resource view for cube texture");
				}

				cubeTextureView = view;
			}

			for (UINT i = 0; i < 6; ++i)
			{
				if (cubeMaterialId[i] != newMaterialids[i])
				{
					cubeMaterialId[i] = newMaterialids[i];

					D3D11_BOX srcbox;
					srcbox.left = 0;
					srcbox.top = 0;
					srcbox.front = 0;
					srcbox.right = materialArray.Width();
					srcbox.bottom = materialArray.Width();
					srcbox.back = 1;

					dc.CopySubresourceRegion(cubeTexture, i, 0, 0, 0, materialArray.Binding().texture, cubeMaterialId[i], &srcbox);
				}
			}
		}
	};

	IDX11CubeTextures* CreateCubeTextureManager(ID3D11Device& device, ID3D11DeviceContext& dc)
	{
		return new CubeTextures(device, dc);
	}
}