#include "dx11.renderer.h"
#include <rococo.renderer.h>

#include <stdarg.h>
#include <wchar.h>
#include <malloc.h>

#ifdef _DEBUG
#pragma comment(lib, "rococo.tiff.debug.lib")
#pragma comment(lib, "rococo.windows.debug.lib")
#pragma comment(lib, "rococo.fonts.debug.lib")
#else
#pragma comment(lib, "rococo.tiff.lib")
#pragma comment(lib, "rococo.windows.lib")
#pragma comment(lib, "rococo.fonts.lib")
#endif

#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "d3d11.lib")

#include <d3d11.h>
#include <vector>
#include <algorithm>

#include "dx11helpers.inl"
#include "dx11dialog.inl"

#include <rococo.imaging.h>
#include <rococo.fonts.h>
#include <rococo.maths.h>
#include <rococo.io.h>

#include <unordered_map>

namespace
{
	using namespace Rococo;

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

	DXGI_SWAP_CHAIN_DESC GetSwapChainDescription(HWND hWnd)
	{
		RECT rect;
		GetClientRect(hWnd, &rect);

		DXGI_SWAP_CHAIN_DESC sd;
		ZeroMemory(&sd, sizeof(sd));
		sd.BufferCount = 1;
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
		sd.SwapEffect = DXGI_SWAP_EFFECT_SEQUENTIAL;
		return sd;
	} 

	class ExpandingBuffer : public IExpandingBuffer
	{
		std::vector<uint8> internalBuffer;
	public:
		ExpandingBuffer(size_t capacity): internalBuffer(capacity)
		{
		}

		virtual const uint8* GetData() const
		{
			if (internalBuffer.empty()) return nullptr;
			else return &internalBuffer[0];
		}

		virtual uint8* GetData()
		{
			if (internalBuffer.empty()) return nullptr;
			else return &internalBuffer[0];
		}

		virtual size_t Length() const
		{
			return internalBuffer.size();
		}

		virtual void Resize(size_t length)
		{
			internalBuffer.resize(length);
		}

		virtual void Free()
		{
			delete this;
		}
	};
}

namespace Rococo
{
	void ShowErrorBox(IException& ex, const wchar_t* caption)
	{
		if (ex.ErrorCode() == 0)
		{
			MessageBox(nullptr, ex.Message(), caption, MB_ICONERROR);
		}
		else
		{
			wchar_t codeMsg[512];
			wchar_t bigMsg[512];
			if (FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, nullptr, ex.ErrorCode(), 0, codeMsg, 512, nullptr) <= 0)
			{
				SafeFormat(bigMsg, _TRUNCATE, L"%s. Code 0x%x", ex.Message(), ex.ErrorCode());
			}
			else
			{
				SafeFormat(bigMsg, _TRUNCATE, L"%s\nCode 0x%x: %s", ex.Message(), ex.ErrorCode(), codeMsg);
			}
			
			MessageBox(nullptr, bigMsg, caption, MB_ICONERROR);
		}
	}

	IExpandingBuffer* CreateExpandingBuffer(size_t initialCapacity)
	{
		return new ExpandingBuffer(initialCapacity);
	}
}

namespace
{
	using namespace Rococo;
	using namespace Rococo::Windows;

	struct UltraClock : public IUltraClock
	{
		ticks hz;
		ticks frameStart;
		ticks start;
		ticks frameDelta;

		UltraClock()
		{
			if (!QueryPerformanceFrequency((LARGE_INTEGER*)&hz))
			{
				Throw(GetLastError(), L"Cannot acquire high performance monitor");
			}

			QueryPerformanceCounter((LARGE_INTEGER*)&start);
		}

		virtual ticks Hz() const { return hz; }
		virtual ticks FrameStart() const { return frameStart; }
		virtual ticks Start() const { return start; }
		virtual ticks FrameDelta() const { return frameDelta; }
	};

	D3D11_INPUT_ELEMENT_DESC guiVertexDesc[] =
	{
		{ "position",	0, DXGI_FORMAT_R32_FLOAT,			0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "position",	1, DXGI_FORMAT_R32_FLOAT,			0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "position",	2, DXGI_FORMAT_R32_FLOAT,			0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "position",	3, DXGI_FORMAT_R32_FLOAT,			0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "color",		0, DXGI_FORMAT_R8G8B8A8_UNORM,      0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "texcoord",	0, DXGI_FORMAT_R32G32B32_FLOAT,     0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	const D3D11_INPUT_ELEMENT_DESC* const GetGuiVertexDesc()
	{
		static_assert(sizeof(GuiVertex) == 32, "Gui vertex data was not 32 bytes wide");
		return guiVertexDesc;
	}

	const uint32 NumberOfGuiVertexElements()
	{
		static_assert(sizeof(guiVertexDesc) / sizeof(D3D11_INPUT_ELEMENT_DESC) == 6, "Vertex data was not 3 fields");
		return sizeof(guiVertexDesc) / sizeof(D3D11_INPUT_ELEMENT_DESC);
	}

	D3D11_INPUT_ELEMENT_DESC objectVertexDesc[] =
	{
		{ "position",	0, DXGI_FORMAT_R32G32B32_FLOAT,	0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "normal",		0, DXGI_FORMAT_R32G32B32_FLOAT,	0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "color",		0, DXGI_FORMAT_R8G8B8A8_UNORM,	0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "color",		1, DXGI_FORMAT_R8G8B8A8_UNORM,	0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "texcoord",	0, DXGI_FORMAT_R32G32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	const uint32 NumberOfObjectVertexElements()
	{
		static_assert(sizeof(objectVertexDesc) / sizeof(D3D11_INPUT_ELEMENT_DESC) == 5, "Vertex data was not 5 fields");
		return sizeof(objectVertexDesc) / sizeof(D3D11_INPUT_ELEMENT_DESC);
	}

	struct DX11Shader
	{
		std::wstring name;
	};

	struct DX11VertexShader : public DX11Shader
	{
		AutoRelease<ID3D11InputLayout> inputLayout;
		AutoRelease<ID3D11VertexShader> vs;
	};

	struct DX11PixelShader : public DX11Shader
	{
		AutoRelease<ID3D11PixelShader> ps;
	};

	struct IAppEventHandler
	{
		virtual void BindMainWindow(HWND hWnd) = 0;
		virtual bool OnClose() = 0;
		virtual void OnKeyboardEvent(const RAWKEYBOARD& k) = 0;
		virtual void OnMouseEvent(const RAWMOUSE& m) = 0;
		virtual void OnSize(HWND hWnd, const Vec2i& span, RESIZE_TYPE type) = 0;
	};

	void ExpandZoneToContain(GuiRect& rect, const Vec2i& p)
	{
		if (p.x < rect.left) rect.left = p.x;
		if (p.x > rect.right) rect.right = p.x;
		if (p.y < rect.top) rect.top = p.y;
		if (p.y > rect.bottom) rect.bottom = p.y;
	}

	void ExpandZoneToContain(Quad& rect, const Vec2& p)
	{
		if (p.x < rect.left) rect.left = p.x;
		if (p.x > rect.right) rect.right = p.x;
		if (p.y < rect.top) rect.top = p.y;
		if (p.y > rect.bottom) rect.bottom = p.y;
	}

	class SpanEvaluator : public Fonts::IGlyphRenderer
	{
	public:
		Quad renderZone;

		SpanEvaluator() : renderZone(10000, 10000, -10000, -10000)
		{

		}

		virtual void DrawGlyph(const Vec2& t0, const Vec2& p, float dx, float dy, Fonts::FontColour colour)
		{
			ExpandZoneToContain(renderZone, Vec2{ p.x, p.y });
			ExpandZoneToContain(renderZone, Vec2{ p.x + dx, p.y + dy });
		}

		Vec2 Span() const
		{
			return Rococo::Span(renderZone);
		}
	};

	struct GuiScale
	{
		float OOScreenWidth;
		float OOScreenHeight;
		float OOFontWidth;
		float OOFontHeight;
	};

	template<class T> ID3D11Buffer* CreateConstantBuffer(ID3D11Device& device)
	{
		D3D11_BUFFER_DESC desc;
		desc.ByteWidth = sizeof(T);
		desc.Usage = D3D11_USAGE_DYNAMIC;
		desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		desc.MiscFlags = 0;
		desc.StructureByteStride = 0;

		// Create the buffer.
		ID3D11Buffer* buffer = nullptr;
		VALIDATEDX11(device.CreateBuffer(&desc, nullptr, &buffer));
		return buffer;
	}

	template<class T> ID3D11Buffer* CreateDynamicVertexBuffer(ID3D11Device& device, size_t capacity)
	{
		size_t nBytes = sizeof(T) * capacity;
		if (capacity > std::numeric_limits<UINT>::max())
		{
			Throw(E_INVALIDARG, L"CreateDynamicVertexBuffer failed - capacity too large");
		}

		D3D11_BUFFER_DESC bufferDesc;
		bufferDesc.ByteWidth = (UINT)nBytes;
		bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
		bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		bufferDesc.MiscFlags = 0;
		bufferDesc.StructureByteStride = 0;

		ID3D11Buffer* buffer;
		VALIDATEDX11(device.CreateBuffer(&bufferDesc, nullptr, &buffer));

		return buffer;
	}

	template<class T> ID3D11Buffer* CreateImmutableVertexBuffer(ID3D11Device& device, const T* vertices, size_t capacity)
	{
		size_t nBytes = sizeof(T) * capacity;
		if (capacity > std::numeric_limits<UINT>::max())
		{
			Throw(E_INVALIDARG, L"CreateImmutableVertexBuffer failed - capacity too large");
		}

		D3D11_BUFFER_DESC bufferDesc;
		bufferDesc.ByteWidth = (UINT)nBytes;
		bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
		bufferDesc.CPUAccessFlags = 0;
		bufferDesc.MiscFlags = 0;
		bufferDesc.StructureByteStride = 0;

		D3D11_SUBRESOURCE_DATA initial;
		initial.pSysMem = vertices;

		ID3D11Buffer* buffer;
		VALIDATEDX11(device.CreateBuffer(&bufferDesc, &initial, &buffer));

		return buffer;
	}

	void CopyStructureToBuffer(ID3D11DeviceContext& dc, ID3D11Buffer* dest, const void* src, size_t nBytes)
	{
		D3D11_MAPPED_SUBRESOURCE bufferMap;
		VALIDATEDX11(dc.Map(dest, 0, D3D11_MAP_WRITE_DISCARD, 0, &bufferMap));
		memcpy(bufferMap.pData, src, nBytes);
		dc.Unmap(dest, 0);
	}

	template<class T> void CopyStructureToBuffer(ID3D11DeviceContext& dc, ID3D11Buffer* dest, const T& t)
	{
		CopyStructureToBuffer(dc, dest, &t, sizeof(T));
	}

	class DX11AppRenderer : public IRenderer, public IRenderContext, public IGuiRenderContext, public Fonts::IGlyphRenderer
	{
	private:
		ID3D11Device& device;
		ID3D11DeviceContext& dc;
		IDXGIFactory& factory;

		AutoRelease<IDXGISwapChain> mainSwapChain;
		AutoRelease<ID3D11RenderTargetView> mainBackBufferView;
		AutoRelease<ID3D11DepthStencilView> depthStencilView;

		std::vector<DX11VertexShader*> vertexShaders;
		std::vector<DX11PixelShader*> pixelShaders;
		AutoRelease<ID3D11Buffer> guiBuffer;

		std::vector<GuiVertex> guiVertices;

		enum { GUI_BUFFER_VERTEX_CAPACITY = 3 * 512 };

		AutoRelease<ID3D11Texture2D> fontTexture;
		AutoRelease<ID3D11SamplerState> spriteSampler;
		AutoRelease<ID3D11ShaderResourceView> fontBinding;
		AutoRelease<ID3D11RasterizerState> spriteRaterizering;
		AutoRelease<ID3D11RasterizerState> objectRaterizering;
		AutoRelease<ID3D11BlendState> alphaBlend;
		AutoRelease<ID3D11BlendState> disableBlend;

		Fonts::IFontSupervisor* fonts;
		Quad clipRect;

		AutoRelease<ID3D11Buffer> vector4Buffer;
		AutoRelease<ID3D11Buffer> globalStateBuffer;

		RAWMOUSE lastMouseEvent;
		Vec2i screenSpan;

		HWND hRenderWindow;

		ID_VERTEX_SHADER idGuiVS;
		ID_PIXEL_SHADER idGuiPS;

		ID_VERTEX_SHADER idObjVS;
		ID_PIXEL_SHADER idObjPS;

		AutoRelease<ID3D11Buffer> instanceBuffer;

		AutoRelease<ID3D11DepthStencilState> guiDepthState;
		AutoRelease<ID3D11DepthStencilState> objDepthState;

		struct TextureBind
		{
			ID3D11Texture2D* texture;
			ID3D11ShaderResourceView* textureView;
		};

		std::vector<TextureBind> textures;
		std::unordered_map<std::wstring, ID_TEXTURE> mapNameToTexture;

		struct Cursor
		{
			ID_TEXTURE bitmapId;
			Vec2 uvTopLeft;
			Vec2 uvBottomRight;
			Vec2i hotspotOffset;
		} cursor;

		ID_TEXTURE lastTextureId;
	public:
		Windows::IWindow* window;

		DX11AppRenderer(ID3D11Device& _device, ID3D11DeviceContext& _dc, IDXGIFactory& _factory, IInstallation& installation) :
			device(_device), dc(_dc), factory(_factory), fonts(nullptr), clipRect(-10000.0f, -10000.0f, 10000.0f, 10000.0f), hRenderWindow(0),
			window(nullptr), cursor{ 0, {0,0}, {1,1}, {0,0} }
		{
			static_assert(GUI_BUFFER_VERTEX_CAPACITY % 3 == 0, "Capacity must be divisible by 3");
			guiBuffer = CreateDynamicVertexBuffer<GuiVertex>(device, GUI_BUFFER_VERTEX_CAPACITY);

			{
				D3D11_SAMPLER_DESC spriteSamplerDesc;
				ZeroMemory(&spriteSamplerDesc, sizeof(spriteSamplerDesc));
				spriteSamplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
				spriteSamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
				spriteSamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
				spriteSamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
				spriteSamplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
				spriteSamplerDesc.MinLOD = 0;
				spriteSamplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

				VALIDATEDX11(device.CreateSamplerState(&spriteSamplerDesc, &spriteSampler));
			}

			{
				D3D11_DEPTH_STENCIL_DESC desc;
				ZeroMemory(&desc, sizeof(desc));
				desc.DepthEnable = TRUE;
				desc.DepthFunc = D3D11_COMPARISON_LESS;
				desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
				VALIDATEDX11(device.CreateDepthStencilState(&desc, &objDepthState));
			}

			{
				D3D11_DEPTH_STENCIL_DESC desc;
				ZeroMemory(&desc, sizeof(desc));
				VALIDATEDX11(device.CreateDepthStencilState(&desc, &guiDepthState));
			}

			{
				D3D11_RASTERIZER_DESC spriteRenderingDesc;
				spriteRenderingDesc.FillMode = D3D11_FILL_SOLID;
				spriteRenderingDesc.CullMode = D3D11_CULL_NONE;
				spriteRenderingDesc.FrontCounterClockwise = TRUE;
				spriteRenderingDesc.DepthBias = 0;
				spriteRenderingDesc.DepthBiasClamp = 0.0f;
				spriteRenderingDesc.SlopeScaledDepthBias = 0.0f;
				spriteRenderingDesc.DepthClipEnable = FALSE;
				spriteRenderingDesc.ScissorEnable = FALSE;
				spriteRenderingDesc.MultisampleEnable = FALSE;

				VALIDATEDX11(device.CreateRasterizerState(&spriteRenderingDesc, &spriteRaterizering));
			}

			{
				D3D11_RASTERIZER_DESC objRenderingDesc;
				objRenderingDesc.FillMode = D3D11_FILL_SOLID;
				objRenderingDesc.CullMode = D3D11_CULL_BACK;
				objRenderingDesc.FrontCounterClockwise = FALSE;
				objRenderingDesc.DepthBias = 0;
				objRenderingDesc.DepthBiasClamp = 0.0f;
				objRenderingDesc.SlopeScaledDepthBias = 0.0f;
				objRenderingDesc.DepthClipEnable = FALSE;
				objRenderingDesc.ScissorEnable = FALSE;
				objRenderingDesc.MultisampleEnable = FALSE;

				VALIDATEDX11(device.CreateRasterizerState(&objRenderingDesc, &objectRaterizering));
			}

			{
				D3D11_BLEND_DESC alphaBlendDesc;
				ZeroMemory(&alphaBlendDesc, sizeof(alphaBlendDesc));

				auto& blender = alphaBlendDesc.RenderTarget[0];

				blender.SrcBlendAlpha = D3D11_BLEND_ZERO;
				blender.SrcBlend = D3D11_BLEND_SRC_ALPHA;
				blender.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
				blender.DestBlendAlpha = D3D11_BLEND_ZERO;
				blender.BlendOpAlpha = D3D11_BLEND_OP_ADD;
				blender.BlendEnable = TRUE;
				blender.BlendOp = D3D11_BLEND_OP_ADD;
				blender.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
				VALIDATEDX11(device.CreateBlendState(&alphaBlendDesc, &alphaBlend));
			}

			{
				D3D11_BLEND_DESC disableBlendDesc;
				ZeroMemory(&disableBlendDesc, sizeof(disableBlendDesc));
				disableBlendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
				VALIDATEDX11(device.CreateBlendState(&disableBlendDesc, &disableBlend));
			}

			AutoFree<IExpandingBuffer> fontFile(CreateExpandingBuffer(64_megabytes));
			const wchar_t* fontName = L"!font1.tif";
			installation.LoadResource(fontName, *fontFile, 64_megabytes);

			if (fontFile->GetData() == nullptr) Throw(0, L"The font file %s was blank", fontName);

			struct ANON : public Imaging::IImageLoadEvents
			{
				const wchar_t* fontName;
				ID3D11Device* device;
				AutoRelease<ID3D11Texture2D>& fontTexture;

				ANON(AutoRelease<ID3D11Texture2D>& _fontTexture) : fontTexture(_fontTexture) {}

				virtual void OnError(const char* message)
				{
					Throw(0, L"Could not load %s: %s", fontName, message);
				}

				virtual void OnARGBImage(const Vec2i& span, const Imaging::F_A8R8G8B8* data)
				{

				}

				virtual void OnAlphaImage(const Vec2i& span, const uint8* data)
				{
					D3D11_TEXTURE2D_DESC alphaImmutableSprite;
					alphaImmutableSprite.Width = span.x;
					alphaImmutableSprite.Height = span.y;
					alphaImmutableSprite.MipLevels = 1;
					alphaImmutableSprite.ArraySize = 1;
					alphaImmutableSprite.Format = DXGI_FORMAT_R8_UNORM;
					alphaImmutableSprite.SampleDesc.Count = 1;
					alphaImmutableSprite.SampleDesc.Quality = 0;
					alphaImmutableSprite.Usage = D3D11_USAGE_IMMUTABLE;
					alphaImmutableSprite.BindFlags = D3D11_BIND_SHADER_RESOURCE;
					alphaImmutableSprite.CPUAccessFlags = 0;
					alphaImmutableSprite.MiscFlags = 0;

					D3D11_SUBRESOURCE_DATA level0Def;
					level0Def.pSysMem = data;
					level0Def.SysMemPitch = span.x;
					level0Def.SysMemSlicePitch = 0;

					VALIDATEDX11(device->CreateTexture2D(&alphaImmutableSprite, &level0Def, &fontTexture));
				}
			} anon(fontTexture);

			anon.fontName = fontName;
			anon.device = &device;

			Rococo::Imaging::DecompressTiff(anon, fontFile->GetData(), fontFile->Length());

			D3D11_SHADER_RESOURCE_VIEW_DESC desc;
			ZeroMemory(&desc, sizeof(desc));

			desc.Texture2D.MipLevels = 1;
			desc.Texture2D.MostDetailedMip = 0;

			desc.Format = DXGI_FORMAT_UNKNOWN;
			desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;

			VALIDATEDX11(device.CreateShaderResourceView(fontTexture, &desc, &fontBinding));

			const wchar_t* csvName = L"!font1.csv";
			installation.LoadResource(csvName, *fontFile, 256_kilobytes);

			fonts = Fonts::LoadFontCSV(csvName, (const char*)fontFile->GetData(), fontFile->Length());

			AutoFree<IExpandingBuffer> shaderCode(CreateExpandingBuffer(128_kilobytes));

			// Create the buffer.
			vector4Buffer = CreateConstantBuffer<Vec4>(device);

			GuiScale nullVector{ 0,0,0,0 };
			CopyStructureToBuffer(dc, vector4Buffer, nullVector);

			globalStateBuffer = CreateConstantBuffer<GlobalState>(device);

			installation.LoadResource(L"!gui.vs", *shaderCode, 64_kilobytes);
			idGuiVS = CreateGuiVertexShader(L"gui.vs", shaderCode->GetData(), shaderCode->Length());

			installation.LoadResource(L"!gui.ps", *shaderCode, 64_kilobytes);
			idGuiPS = CreatePixelShader(L"gui.ps", shaderCode->GetData(), shaderCode->Length());

			installation.LoadResource(L"!object.vs", *shaderCode, 64_kilobytes);
			idObjVS = CreateObjectVertexShader(L"object.vs", shaderCode->GetData(), shaderCode->Length());

			installation.LoadResource(L"!object.ps", *shaderCode, 64_kilobytes);
			idObjPS = CreatePixelShader(L"object.ps", shaderCode->GetData(), shaderCode->Length());

			instanceBuffer = CreateConstantBuffer<ObjectInstance>(device);
		}

		~DX11AppRenderer()
		{
			ClearContext();

			if (fonts)
			{
				fonts->Free();
			}

			for (auto& x : meshBuffers)
			{
				x.dx11Buffer->Release();
			}

			for (auto& x : vertexShaders)
			{
				delete x;
			}

			for (auto& x : pixelShaders)
			{
				delete x;
			}

			for (auto& t : textures)
			{
				t.texture->Release();
				t.textureView->Release();
			}
		}

		virtual void SetMeshTexture(ID_TEXTURE textureId, int textureIndex)
		{
			size_t index = textureId - 1;
			if (index >= textures.size())
			{
				Throw(0, L"Bad texture id");
			}

			auto& t = textures[index];

			D3D11_TEXTURE2D_DESC desc;
			t.texture->GetDesc(&desc);

			if (textureId != lastTextureId)
			{
				lastTextureId = textureId;
				dc.PSSetShaderResources(textureIndex, 1, &t.textureView);
			}
		}

		virtual IRenderer& Renderer()
		{
			return *this;
		}

		virtual ID_TEXTURE LoadTexture(IBuffer& buffer, const wchar_t* uniqueName)
		{
			auto i = mapNameToTexture.find(uniqueName);
			if (i != mapNameToTexture.end())
			{
				return i->second;
			}

			struct ANON : public Imaging::IImageLoadEvents
			{
				ID3D11Device* device;
				ID3D11Texture2D* texture;

				ANON()  {}

				virtual void OnError(const char* message)
				{
					Throw(0, L"Could not parse image: %s", message);
				}

				virtual void OnARGBImage(const Vec2i& span, const Imaging::F_A8R8G8B8* data)
				{
					D3D11_TEXTURE2D_DESC alphaImmutableSprite;
					alphaImmutableSprite.Width = span.x;
					alphaImmutableSprite.Height = span.y;
					alphaImmutableSprite.MipLevels = 1;
					alphaImmutableSprite.ArraySize = 1;
					alphaImmutableSprite.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
					alphaImmutableSprite.SampleDesc.Count = 1;
					alphaImmutableSprite.SampleDesc.Quality = 0;
					alphaImmutableSprite.Usage = D3D11_USAGE_IMMUTABLE;
					alphaImmutableSprite.BindFlags = D3D11_BIND_SHADER_RESOURCE;
					alphaImmutableSprite.CPUAccessFlags = 0;
					alphaImmutableSprite.MiscFlags = 0;

					D3D11_SUBRESOURCE_DATA level0Def;
					level0Def.pSysMem = data;
					level0Def.SysMemPitch = span.x * sizeof(RGBAb);
					level0Def.SysMemSlicePitch = 0;

					VALIDATEDX11(device->CreateTexture2D(&alphaImmutableSprite, &level0Def, &texture));
				}

				virtual void OnAlphaImage(const Vec2i& span, const uint8* data)
				{
					Throw(0, L"Alpha images are not supported");
				}
			} anon;

			anon.device = &device;
			anon.texture = nullptr;

			Rococo::Imaging::DecompressTiff(anon, buffer.GetData(), buffer.Length());

			if (anon.texture == nullptr)
			{
				Throw(0, L"Could not parse Tiff file");
			}

			D3D11_SHADER_RESOURCE_VIEW_DESC desc;
			ZeroMemory(&desc, sizeof(desc));

			desc.Texture2D.MipLevels = 1;
			desc.Texture2D.MostDetailedMip = 0;

			desc.Format = DXGI_FORMAT_UNKNOWN;
			desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;

			ID3D11ShaderResourceView* textureView;
			HRESULT hr = device.CreateShaderResourceView(anon.texture, &desc, &textureView);
			if (FAILED(hr))
			{
				Throw(0, L"device.CreateShaderResourceView failed.");
				anon.texture->Release();
			}
			
			textures.push_back({ anon.texture, textureView });
			mapNameToTexture.insert(std::make_pair(uniqueName, textures.size()));
			return textures.size();
		}

		virtual auto SelectTexture(ID_TEXTURE id) -> Vec2i
		{
			size_t index = id - 1;
			if (index >= textures.size())
			{
				Throw(0, L"Bad texture id");
			}
			
			auto& t = textures[index];

			D3D11_TEXTURE2D_DESC desc;
			t.texture->GetDesc(&desc);

			if (id != lastTextureId)
			{
				FlushLayer();
				lastTextureId = id;
				dc.PSSetShaderResources(1, 1, &t.textureView);
			}

			return Vec2i{ (int32) desc.Width, (int32) desc.Height };
		}

		void SyncViewport()
		{
			RECT rect;
			GetClientRect(hRenderWindow, &rect);

			D3D11_VIEWPORT viewport;
			ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));

			viewport.TopLeftX = 0;
			viewport.TopLeftY = 0;
			viewport.Width = FLOAT(rect.right - rect.left);
			viewport.Height = FLOAT(rect.bottom - rect.top);
			viewport.MinDepth = 0.0f;
			viewport.MaxDepth = 1.0f;

			dc.RSSetViewports(1, &viewport);

			screenSpan.x = (int32)viewport.Width;
			screenSpan.y = (int32)viewport.Height;
		}

		void BindMainWindow(HWND hRenderWindow)
		{
			this->hRenderWindow = hRenderWindow;

			DXGI_SWAP_CHAIN_DESC swapChainDesc = GetSwapChainDescription(hRenderWindow);
			VALIDATEDX11(factory.CreateSwapChain((ID3D11Device*)&device, &swapChainDesc, &mainSwapChain));

			AutoRelease<ID3D11Texture2D> backBuffer;
			VALIDATEDX11(mainSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer));

			VALIDATEDX11(device.CreateRenderTargetView(backBuffer, nullptr, &mainBackBufferView));

			auto depthDesc = GetDepthDescription(hRenderWindow);

			AutoRelease<ID3D11Texture2D> depthBuffer;
			VALIDATEDX11(device.CreateTexture2D(&depthDesc, nullptr, &depthBuffer));
			VALIDATEDX11(device.CreateDepthStencilView(depthBuffer, nullptr, &depthStencilView));

			SyncViewport();
		}

		ID_VERTEX_SHADER CreateVertexShader(const wchar_t* name, const byte* shaderCode, size_t shaderLength, const D3D11_INPUT_ELEMENT_DESC* vertexDesc, UINT nElements)
		{
			if (name == nullptr || wcslen(name) > 1024) Throw(0, L"Bad <name> for vertex shader");
			if (shaderCode == nullptr || shaderLength < 4 || shaderLength > 65536) Throw(0, L"Bad shader code for vertex shader %s", name);

			DX11VertexShader* shader = new DX11VertexShader;
			HRESULT hr = device.CreateInputLayout(vertexDesc, nElements, shaderCode, shaderLength, &shader->inputLayout);
			if FAILED(hr)
			{
				delete shader;
				Throw(hr, L"device.CreateInputLayout failed with shader %s", name);
				return 0;
			}

			hr = device.CreateVertexShader(shaderCode, shaderLength, nullptr, &shader->vs);
			if FAILED(hr)
			{
				delete shader;
				Throw(hr, L"device.CreateVertexShader failed with shader %s", name);
				return 0;
			}

			shader->name = name;
			vertexShaders.push_back(shader);
			return (ID_VERTEX_SHADER)vertexShaders.size() - 1;
		}

		ID_VERTEX_SHADER CreateGuiVertexShader(const wchar_t* name, const byte* shaderCode, size_t shaderLength)
		{
			return CreateVertexShader(name, shaderCode, shaderLength, GetGuiVertexDesc(), NumberOfGuiVertexElements());
		}

		virtual ID_VERTEX_SHADER CreateObjectVertexShader(const wchar_t* name, const uint8* shaderCode, size_t shaderLength)
		{
			return CreateVertexShader(name, shaderCode, shaderLength, objectVertexDesc, NumberOfObjectVertexElements());
		}

		ID_PIXEL_SHADER CreatePixelShader(const wchar_t* name, const byte* shaderCode, size_t shaderLength)
		{
			if (name == nullptr || wcslen(name) > 1024) Throw(0, L"Bad <name> for pixel shader");
			if (shaderCode == nullptr || shaderLength < 4 || shaderLength > 16384) Throw(0, L"Bad shader code for pixel shader %s", name);

			DX11PixelShader* shader = new DX11PixelShader;
			HRESULT hr = device.CreatePixelShader(shaderCode, shaderLength, nullptr, &shader->ps);
			if FAILED(hr)
			{
				delete shader;
				Throw(hr, L"device.CreatePixelShader failed with shader %s", name);
				return 0;
			}

			shader->name = name;
			pixelShaders.push_back(shader);
			return (ID_PIXEL_SHADER)pixelShaders.size() - 1;
		}

		void RenderText(const Vec2i& pos, Fonts::IDrawTextJob& job)
		{
			char stackBuffer[128];
			Fonts::IGlyphRenderPipeline* pipeline = Fonts::CreateGlyphRenderPipeline(stackBuffer, sizeof(stackBuffer), *this);
			RouteDrawTextBasic(pos, job, *fonts, *pipeline, clipRect);
		}

		Vec2i EvalSpan(const Vec2i& pos, Fonts::IDrawTextJob& job)
		{
			char stackBuffer[128];
			SpanEvaluator spanEvaluator;
			Fonts::IGlyphRenderPipeline* pipeline = Fonts::CreateGlyphRenderPipeline(stackBuffer, sizeof(stackBuffer), spanEvaluator);
			RouteDrawTextBasic(pos, job, *fonts, *pipeline, clipRect);

			return Quantize(spanEvaluator.Span());
		}

		void UseShaders(ID_VERTEX_SHADER vid, ID_PIXEL_SHADER pid)
		{
			if (vid >= vertexShaders.size()) Throw(0, L"Bad vertex shader Id in call to UseShaders");
			if (pid >= pixelShaders.size()) Throw(0, L"Bad pixel shader Id in call to UseShaders");

			auto& vs = *vertexShaders[vid];
			auto& ps = *pixelShaders[pid];

			dc.IASetInputLayout(vs.inputLayout);
			dc.VSSetShader(vs.vs, nullptr, 0);
			dc.PSSetShader(ps.ps, nullptr, 0);
		}

		void ClearContext()
		{
			dc.IASetInputLayout(nullptr);
			dc.VSSetShader(nullptr, nullptr, 0);
			dc.PSSetShader(nullptr, nullptr, 0);
			dc.PSSetShaderResources(0, 0, nullptr);
		}

		void SwitchToWindowMode()
		{
			BOOL isFullScreen;
			AutoRelease<IDXGIOutput> output;
			if SUCCEEDED(mainSwapChain->GetFullscreenState(&isFullScreen, &output))
			{
				if (isFullScreen)
				{
					mainSwapChain->SetFullscreenState(false, nullptr);
				}
			}
		}

		void OnMouseEvent(const RAWMOUSE& m)
		{
			lastMouseEvent = m;
		}

		virtual void GetGuiMetrics(GuiMetrics& metrics) const
		{
			POINT p;
			GetCursorPos(&p);
			ScreenToClient(hRenderWindow, &p);

			metrics.cursorPosition = Vec2i{ p.x, p.y };
			metrics.screenSpan = screenSpan;
		}

		struct MeshBuffer
		{
			ID3D11Buffer* dx11Buffer;
			UINT numberOfVertices;
			D3D_PRIMITIVE_TOPOLOGY topology;
		};

		std::vector<MeshBuffer> meshBuffers;

		virtual ID_MESH CreateTriangleMesh(const ObjectVertex* vertices, uint32 nVertices)
		{
			ID3D11Buffer* meshBuffer = CreateImmutableVertexBuffer(device, vertices, nVertices);
			meshBuffers.push_back(MeshBuffer{ meshBuffer, nVertices, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST });
			int32 index = (int32) meshBuffers.size();
			return index-1;
		}

		virtual void UpdateMesh(ID_MESH rendererId, const ObjectVertex* vertices, uint32 nVertices)
		{
			if (rendererId >= meshBuffers.size())
			{
				Throw(E_INVALIDARG, L"renderer.UpdateMesh(ID_MESH id, ....) - Bad id ");
			}

			ID3D11Buffer* newMesh = CreateImmutableVertexBuffer(device, vertices, nVertices);
			meshBuffers[rendererId].numberOfVertices = nVertices;
			meshBuffers[rendererId].dx11Buffer->Release();
			meshBuffers[rendererId].dx11Buffer = newMesh;
		}

		virtual void Draw(ID_MESH id, const ObjectInstance* instances, uint32 nInstances)
		{
			if (id >= meshBuffers.size()) Throw(E_INVALIDARG, L"renderer.DrawObject(ID_MESH id) - Bad id ");

			auto& buffer = meshBuffers[id];

			ID3D11Buffer* buffers[] = { buffer.dx11Buffer };

			dc.IASetPrimitiveTopology(buffer.topology);
			
			UINT strides[] = { sizeof(ObjectVertex) };
			UINT offsets[]{ 0 };
			dc.IASetVertexBuffers(0, 1, buffers, strides, offsets);

			for (uint32 i = 0; i < nInstances; i++ )
			{
				// dc.DrawInstances crashed the debugger, replace with single instance render call for now
				CopyStructureToBuffer(dc, instanceBuffer, instances + i, sizeof(ObjectInstance));
				dc.VSSetConstantBuffers(1, 1, &instanceBuffer);
				dc.Draw(buffer.numberOfVertices, 0);
				dc.VSSetConstantBuffers(0, 0, nullptr);
			}
		}

		virtual Windows::IWindow& Window()
		{
			return *window;
		}

		GlobalState g;

		virtual void SetGlobalState(const GlobalState& gs)
		{
			CopyStructureToBuffer(dc, globalStateBuffer, gs);
			dc.VSSetConstantBuffers(0, 1, &globalStateBuffer);
		}

		void DrawCursor()
		{
			if (cursor.bitmapId)
			{
				GuiMetrics metrics;
				GetGuiMetrics(metrics);

				Vec2i span = SelectTexture(cursor.bitmapId);

				float u0 = cursor.uvTopLeft.x;
				float u1 = cursor.uvBottomRight.x;
				float v0 = cursor.uvTopLeft.y;
				float v1 = cursor.uvBottomRight.y;

				Vec2 p { (float) metrics.cursorPosition.x, (float) metrics.cursorPosition.y };

				float x0 = p.x + cursor.hotspotOffset.x;
				float x1 = x0 + (float)span.x;
				float y0 = p.y + cursor.hotspotOffset.y;
				float y1 = y0 + (float)span.y;

				GuiVertex quad[6]
				{
					GuiVertex{ x0, y0, 0.0f, 0.0f, RGBAb(128,0,0), u0, v0, 0.0f },
					GuiVertex{ x1, y0, 0.0f, 0.0f, RGBAb(0,128,0), u1, v0, 0.0f },
					GuiVertex{ x1, y1, 0.0f, 0.0f, RGBAb(0,0,128), u1, v1, 0.0f },
					GuiVertex{ x1, y1, 0.0f, 0.0f, RGBAb(128,0,0), u1, v1, 0.0f },
					GuiVertex{ x0, y1, 0.0f, 0.0f, RGBAb(0,128,0), u0, v1, 0.0f },
					GuiVertex{ x0, y0, 0.0f, 0.0f, RGBAb(0,0,128), u0, v0, 0.0f }
				};

				AddTriangle(quad);
				AddTriangle(quad + 3);

				SetCursor(0);
			}
			else
			{
				SetCursor(LoadCursor(nullptr, IDC_ARROW));
			}
		}

		virtual void Render(IScene& scene)
		{
			if (mainBackBufferView.IsNull()) return;

			lastTextureId = -1;

			dc.OMSetRenderTargets(1, &mainBackBufferView, depthStencilView);

			RGBA clearColour = scene.GetClearColour();

			if (clearColour.alpha > 0)
			{
				dc.ClearRenderTargetView(mainBackBufferView, (const FLOAT*)&clearColour);
				dc.ClearDepthStencilView(depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
			}

			FLOAT blendFactorUnused[] = { 0,0,0,0 };
			dc.OMSetBlendState(disableBlend, blendFactorUnused, 0xffffffff);
			UseShaders(idObjVS, idObjPS);

			dc.RSSetState(objectRaterizering);
			dc.OMSetDepthStencilState(objDepthState, 0);

			scene.RenderObjects(*this);

			UseShaders(idGuiVS, idGuiPS);

			dc.PSSetSamplers(0, 1, &spriteSampler);
			dc.PSSetShaderResources(0, 1, &fontBinding);

			dc.RSSetState(spriteRaterizering);

			dc.OMSetBlendState(alphaBlend, blendFactorUnused, 0xffffffff);

			GuiScale guiScaleVector;
			guiScaleVector.OOScreenWidth = 1.0f / screenSpan.x;
			guiScaleVector.OOScreenHeight = 1.0f / screenSpan.y;
			guiScaleVector.OOFontWidth = fonts->TextureSpan().z;
			guiScaleVector.OOFontHeight = fonts->TextureSpan().w;

			CopyStructureToBuffer(dc, vector4Buffer, &guiScaleVector, sizeof(GuiScale));

			dc.VSSetConstantBuffers(0, 1, &vector4Buffer);
			dc.IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			dc.OMSetDepthStencilState(guiDepthState, 0);
			scene.RenderGui(*this);

			FlushLayer();

			DrawCursor();

			FlushLayer();

			dc.PSSetShaderResources(0, 0, nullptr);
			dc.PSSetSamplers(0, 0, nullptr);

			mainSwapChain->Present(1, 0);

			dc.OMSetDepthStencilState(nullptr, 0);
			dc.OMSetRenderTargets(0, nullptr, nullptr);
			dc.IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
		}

		void AddTriangle(const GuiVertex triangle[3])
		{
			guiVertices.push_back(triangle[0]);
			guiVertices.push_back(triangle[1]);
			guiVertices.push_back(triangle[2]);
		}

		RGBAb FontColourToSysColour(Fonts::FontColour colour)
		{
			RGBAb* pCol = (RGBAb*)&colour;
			return *pCol;
		}

		virtual void DrawGlyph(const Vec2& uvTopLeft, const Vec2& posTopLeft, float dx, float dy, Fonts::FontColour fcolour)
		{
			float x = posTopLeft.x;
			float y = posTopLeft.y;

			RGBAb colour = FontColourToSysColour(fcolour);

			guiVertices.push_back(GuiVertex{ x,           y, 1.0f, 1.0f, (RGBAb)colour, uvTopLeft.x,      uvTopLeft.y,      0.0f }); // topLeft
			guiVertices.push_back(GuiVertex{ x,      y + dy, 1.0f, 1.0f, (RGBAb)colour, uvTopLeft.x,      uvTopLeft.y + dy, 0.0f }); // bottomLeft
			guiVertices.push_back(GuiVertex{ x + dx, y + dy, 1.0f, 1.0f, (RGBAb)colour, uvTopLeft.x + dx, uvTopLeft.y + dy, 0.0f }); // bottomRigh
			guiVertices.push_back(GuiVertex{ x,           y, 1.0f, 1.0f, (RGBAb)colour, uvTopLeft.x,      uvTopLeft.y,      0.0f }); // topLeft
			guiVertices.push_back(GuiVertex{ x + dx,      y, 1.0f, 1.0f, (RGBAb)colour, uvTopLeft.x + dx, uvTopLeft.y,      0.0f }); // TopRight
			guiVertices.push_back(GuiVertex{ x + dx, y + dy, 1.0f, 1.0f, (RGBAb)colour, uvTopLeft.x + dx, uvTopLeft.y + dy, 0.0f }); // bottomRight
		}

		void FlushLayer()
		{
			size_t nVerticesLeftToRender = guiVertices.size();
			while (nVerticesLeftToRender > 0)
			{
				size_t startIndex = guiVertices.size() - nVerticesLeftToRender;
				size_t chunk = std::min(nVerticesLeftToRender, (size_t)GUI_BUFFER_VERTEX_CAPACITY);

				D3D11_MAPPED_SUBRESOURCE x;
				VALIDATEDX11(dc.Map(guiBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &x));
				memcpy(x.pData, &guiVertices[startIndex], chunk * sizeof(GuiVertex));
				dc.Unmap(guiBuffer, 0);

				UINT stride = sizeof(GuiVertex);
				UINT offset = 0;
				dc.IASetVertexBuffers(0, 1, &guiBuffer, &stride, &offset);
				dc.Draw((UINT)chunk, 0);

				nVerticesLeftToRender -= chunk;
			};

			guiVertices.clear();
		}

		void ResizeBuffers(const Vec2i& span)
		{
			mainBackBufferView.Detach();

			VALIDATEDX11(mainSwapChain->ResizeBuffers(1, span.x, span.y, DXGI_FORMAT_R8G8B8A8_UNORM, 0));

			AutoRelease<ID3D11Texture2D> backBuffer;
			VALIDATEDX11(mainSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer));
			VALIDATEDX11(device.CreateRenderTargetView(backBuffer, nullptr, &mainBackBufferView));

			depthStencilView.Detach();

			AutoRelease<ID3D11Texture2D> depthBuffer;

			auto depthDesc = GetDepthDescription(hRenderWindow);
			VALIDATEDX11(device.CreateTexture2D(&depthDesc, nullptr, &depthBuffer));
			VALIDATEDX11(device.CreateDepthStencilView(depthBuffer, nullptr, &depthStencilView));

			SyncViewport();
		}

		virtual void OnSize(HWND hWnd, const Vec2i& span, RESIZE_TYPE type)
		{
			if (span.x > 0 && span.y > 0)
			{
				D3D11_TEXTURE2D_DESC desc;
				{
					AutoRelease<ID3D11Texture2D> backBuffer;
					VALIDATEDX11(mainSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer));
					backBuffer->GetDesc(&desc);
				}

				if (desc.Width != span.x || desc.Height != span.y)
				{
					ResizeBuffers(span);
				}
			}
		}

		void SetCursorBitmap(ID_TEXTURE idBitmap, Vec2i hotspotOffset, Vec2 uvTopLeft, Vec2 uvBottomRight)
		{
			cursor.bitmapId = idBitmap;
			cursor.uvTopLeft = uvTopLeft;
			cursor.uvBottomRight = uvBottomRight;
			cursor.hotspotOffset = hotspotOffset;
		}
	};

	struct DX11Host
	{
		AutoRelease<IDXGIAdapter> adapter;
		AutoRelease<ID3D11DeviceContext> dc;
		AutoRelease<ID3D11Device> device;
		AutoRelease<IDXGIFactory> factory;
	};

	class MainWindowHandler : public StandardWindowHandler
	{
	private:
		IDialogSupervisor* window;
		IAppEventHandler& eventHandler;
		bool hasFocus;

		MainWindowHandler(IAppEventHandler& _eventHandler) : eventHandler(_eventHandler), window(nullptr), hasFocus(false)
		{

		}

		~MainWindowHandler()
		{
			Rococo::Free(window); // top level windows created with CreateDialogWindow have to be manually freed
		}

		void PostConstruct()
		{
			WindowConfig config;
			SetOverlappedWindowConfig(config, Vec2i{ 800, 600 }, SW_SHOWMAXIMIZED, nullptr, L"DX11 64-bit Rococo API Window", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 0);
			window = Windows::CreateDialogWindow(config, this); // Specify 'this' as our window handler
			eventHandler.BindMainWindow(*window);

			RegisterRawInput();
			hasFocus = true;
		}

		void RegisterRawInput()
		{
			RAWINPUTDEVICE mouseDesc;
			mouseDesc.hwndTarget = *window;
			mouseDesc.dwFlags = 0;
			mouseDesc.usUsage = 0x02;
			mouseDesc.usUsagePage = 0x01;
			if (!RegisterRawInputDevices(&mouseDesc, 1, sizeof(mouseDesc)))
			{
				Throw(GetLastError(), L"RegisterRawInputDevices(&mouseDesc, 1, sizeof(mouseDesc) failed");
			}

			RAWINPUTDEVICE keyboardDesc;
			keyboardDesc.hwndTarget = *window;
			keyboardDesc.dwFlags = 0;
			keyboardDesc.usUsage = 0x06;
			keyboardDesc.usUsagePage = 0x01;
			if (!RegisterRawInputDevices(&keyboardDesc, 1, sizeof(keyboardDesc)))
			{
				Throw(GetLastError(), L"RegisterRawInputDevices(&keyboardDesc, 1, sizeof(keyboardDesc)) failed");
			}
		}
	public:
		// This is our post construct pattern. Allow the constructor to return to initialize the v-tables, then call PostConstruct to create the window 
		static MainWindowHandler* Create(IAppEventHandler& _eventHandler)
		{
			auto m = new MainWindowHandler(_eventHandler);
			m->PostConstruct();
			return m;
		}

		void Free()
		{
			delete this;
		}

		virtual LRESULT OnMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
		{
			switch (uMsg)
			{
			case WM_SETFOCUS:
				hasFocus = true;
				RegisterRawInput();
				break;
			case WM_KILLFOCUS:
				hasFocus = false;
				break;
			}

			return StandardWindowHandler::OnMessage(hWnd, uMsg, wParam, lParam);
		}

		virtual void OnMenuCommand(HWND hWnd, DWORD id)
		{

		}

		LRESULT OnInput(HWND hWnd, WPARAM wParam, LPARAM lParam)
		{
			UINT sizeofBuffer;
			if (NO_ERROR != GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &sizeofBuffer, sizeof(RAWINPUTHEADER)))
			{
				return DefWindowProc(hWnd, WM_INPUT, wParam, lParam);
			}

			// sizeOfBuffer is now correctly set with size of the input buffer
			char* buffer = (char*)_alloca(sizeofBuffer);

			if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, buffer, &sizeofBuffer, sizeof(RAWINPUTHEADER)) == (UINT)-1)
			{
				return DefWindowProc(hWnd, WM_INPUT, wParam, lParam);
			}

			RAWINPUT& raw = *((RAWINPUT*)buffer);
			RAWINPUT* pRaw = &raw;

			if (hasFocus)
			{
				if (raw.header.dwType == RIM_TYPEMOUSE)
				{
					eventHandler.OnMouseEvent(raw.data.mouse);
				}
				else if (raw.header.dwType == RIM_TYPEKEYBOARD)
				{
					eventHandler.OnKeyboardEvent(raw.data.keyboard);
				}

				return 0;
			}
			else
			{
				return DefRawInputProc(&pRaw, 1, sizeof(RAWINPUTHEADER));
			}
		}

		virtual void OnClose(HWND hWnd)
		{
			if (eventHandler.OnClose())
			{
				AutoFree<CountdownConfirmationDialog> confirmDialog(CountdownConfirmationDialog::Create());
				if (IDOK == confirmDialog->DoModal(hWnd, L"DX11 Test Window", L"Quitting application", 8))
				{
					PostQuitMessage(0);
				}
			}
		}

		virtual IDialogSupervisor& Window()
		{
			return *window;
		}

		virtual void OnSize(HWND hWnd, const Vec2i& span, RESIZE_TYPE type)
		{
			eventHandler.OnSize(hWnd, span, type);
		}
	};

	enum { IDSHOWMODAL = 1001 };

	void Create_DX11_0_DebugHost(UINT adapterIndex, DX11Host& host)
	{
		VALIDATEDX11(CreateDXGIFactory(IID_IDXGIFactory, (void**)&host.factory));
		VALIDATEDX11(host.factory->EnumAdapters(adapterIndex, &host.adapter));

		D3D_FEATURE_LEVEL featureLevelNeeded[] = { D3D_FEATURE_LEVEL_11_0 };
		D3D_FEATURE_LEVEL featureLevelFound;

		VALIDATEDX11(D3D11CreateDevice(host.adapter, D3D_DRIVER_TYPE_UNKNOWN, nullptr, D3D11_CREATE_DEVICE_DEBUG,
			featureLevelNeeded, 1, D3D11_SDK_VERSION, &host.device, &featureLevelFound, &host.dc
			));

		if (featureLevelFound != D3D_FEATURE_LEVEL_11_0)
		{
			Throw(0, L"DX 11.0 is required for this application");
		}
	}

	void MainLoop(MainWindowHandler& mainWindow, HANDLE hInstanceLock, IApp& app)
	{
		UltraClock uc;

		ticks lastTick = uc.start;

		DWORD sleepMS = 5;
		MSG msg = { 0 };
		while (msg.message != WM_QUIT)
		{
			DWORD status = MsgWaitForMultipleObjectsEx(1, &hInstanceLock, sleepMS, QS_ALLEVENTS, MWMO_ALERTABLE);

			if (status == WAIT_OBJECT_0)
			{
				ResetEvent(hInstanceLock);
				SetForegroundWindow(mainWindow.Window());
			}

			while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);

				if (msg.message == WM_QUIT)
				{
					return;
				}
			}

			QueryPerformanceCounter((LARGE_INTEGER*)&uc.frameStart);

			uc.frameDelta = uc.frameStart - lastTick;

			sleepMS = app.OnFrameUpdated(uc);

			lastTick = uc.frameStart;
		}
	}
} // anon

namespace Rococo
{
	void CALLBACK RendererMain(HANDLE hInstanceLock, IInstallation& installation, IAppFactory& appFactory)
	{
		DX11Host host;
		Create_DX11_0_DebugHost(0, host);

		DX11AppRenderer renderer(*host.device, *host.dc, *host.factory, installation);

		struct ANON : IAppEventHandler, IEventCallback<SysUnstableArgs>
		{
			DX11AppRenderer* renderer;
			IApp* app;
			IOS* os;

			~ANON()
			{
				os->SetUnstableHandler(nullptr);
			}

			virtual void BindMainWindow(HWND hWnd) 
			{
				renderer->BindMainWindow(hWnd);
			}

			virtual bool OnClose()
			{
				renderer->SwitchToWindowMode();
				return true;
			}

			virtual void OnKeyboardEvent(const RAWKEYBOARD& k)
			{
				if (app) app->OnKeyboardEvent((const KeyboardEvent&)k);
			}

			virtual void OnMouseEvent(const RAWMOUSE& m)
			{
				renderer->OnMouseEvent(m);
				if (app) app->OnMouseEvent((const MouseEvent&) m);
			}

			virtual void OnSize(HWND hWnd, const Vec2i& span, RESIZE_TYPE type)
			{
				renderer->OnSize(hWnd, span, type);
			}

			virtual void OnEvent(SysUnstableArgs& arg)
			{
				renderer->SwitchToWindowMode();
			}

		} eventSink;

		eventSink.renderer = &renderer;
		eventSink.app = nullptr;
		eventSink.os = &installation.OS();

		AutoFree<MainWindowHandler> mainWindowHandler = MainWindowHandler::Create(eventSink);
		SetWindowText(mainWindowHandler->Window(), L"Dystopia, By Mark Anthony Taylor");
		VALIDATEDX11(host.factory->MakeWindowAssociation(mainWindowHandler->Window(), 0));

		renderer.window = &mainWindowHandler->Window();

		installation.OS().SetUnstableHandler(&eventSink);

		AutoFree<IApp> app = appFactory.CreateApp(renderer);
		app->OnCreated();

		eventSink.app = app;

		MainLoop(*mainWindowHandler, hInstanceLock, *app);
	}
}

