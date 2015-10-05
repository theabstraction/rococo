#include "dx11.windows.test.h"
#include <stdarg.h>
#include <wchar.h>
#include <malloc.h>

#include "resource.h"

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

namespace
{
	using namespace Rococo;

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
		return sd;
	} 
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
}

namespace
{
	using namespace Rococo;
	using namespace Rococo::Windows;

	typedef int64 ticks;

	struct IUltraClock
	{
		virtual ticks Hz() const = 0;			// Number of ticks per seconds
		virtual ticks TickStart() const = 0;	// The time of the current tick
		virtual ticks Start() const = 0;		// The time at which the mainloop started
		virtual ticks TickDelta() const = 0;	// The time between the previous tick and the current tick.
	};

	struct UltraClock: public IUltraClock
	{
		ticks hz;
		ticks tickStart;
		ticks start;
		ticks tickDelta;

		UltraClock()
		{
			if (!QueryPerformanceFrequency((LARGE_INTEGER*)&hz))
			{
				Throw(GetLastError(), L"Cannot acquire high performance monitor");
			}

			QueryPerformanceCounter((LARGE_INTEGER*) &start);
		}

		virtual ticks Hz() const { return hz; }
		virtual ticks TickStart() const { return tickStart; }
		virtual ticks Start() const { return start; }
		virtual ticks TickDelta() const { return tickDelta; }
	};

	struct vec4
	{
		float x;
		float y;
		float z;
		float w;
	};

	struct Vec3
	{
		float x;
		float y;
		float z;
	};

	struct RGBAb
	{
		uint8 red;
		uint8 green;
		uint8 blue;
		uint8 alpha;
	};

	struct GuiVertex
	{
		float x;
		float y;
		float saturation;
		float fontBlend;
		RGBAb colour;
		float u;
		float v;
		float unused;
	};

	/*
	struct D3D11_INPUT_ELEMENT_DESC
	{
		LPCSTR SemanticName;
		UINT SemanticIndex;
		DXGI_FORMAT Format;
		UINT InputSlot;
		UINT AlignedByteOffset;
		D3D11_INPUT_CLASSIFICATION InputSlotClass;
		UINT InstanceDataStepRate;
	};
	*/

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

	const uint32 NumberOfVertexElements()
	{
		static_assert(sizeof(guiVertexDesc)/sizeof(D3D11_INPUT_ELEMENT_DESC) == 6, "Vertex data was not 3 fields");
		return sizeof(guiVertexDesc) / sizeof(D3D11_INPUT_ELEMENT_DESC);
	}

	typedef size_t ID_VERTEX_SHADER;
	typedef size_t ID_PIXEL_SHADER;

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

	struct FileHandle
	{
		HANDLE hFile;

		FileHandle(HANDLE _hFile): hFile(_hFile)
		{
		}

		bool IsValid() const
		{
			return hFile != INVALID_HANDLE_VALUE;
		}

		~FileHandle()
		{
			if (IsValid()) CloseHandle(hFile);
		}

		operator HANDLE()
		{
			return hFile;
		}
	};

	enum { UMLIMITED = -1 };

	void LoadFileAbsolute(const wchar_t* filename, std::vector<byte>& data, int64 maxFileSize = UMLIMITED)
	{
		if (filename == nullptr || wcslen(filename) == 0) Throw(E_INVALIDARG, L"LoadFileAbsolute failed: <filename> was blank");
		FileHandle hFile = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
		if (!hFile.IsValid()) Throw(HRESULT_FROM_WIN32(GetLastError()), L"LoadFileAbsolute failed: CreateFile failed for %s", filename);

		LARGE_INTEGER len;
		GetFileSizeEx(hFile, &len);

		if (maxFileSize > 0 && len.QuadPart > maxFileSize)
		{
			Throw(0, L"LoadFileAbsolute failed: File %s was too large at over %ld bytes", filename, maxFileSize);
		}

		data.resize(len.QuadPart);

		int64 bytesLeft = len.QuadPart;
		ptrdiff_t offset = 0;

		while (bytesLeft > 0)
		{
			DWORD chunk = (DWORD)(int32)std::min(bytesLeft, 65536LL);
			DWORD bytesRead = 0;
			if (!ReadFile(hFile, &data[0] + offset, chunk, &bytesRead, nullptr))
			{
				Throw(HRESULT_FROM_WIN32(GetLastError()), L"Error reading file %s", filename);
			}

			if (bytesRead != chunk)
			{
				Throw(0, L"Error reading file %s. Failed to read chunk", filename);
			}

			offset += (ptrdiff_t)chunk;
			bytesLeft -= (int64)chunk;
		}
	}

	struct IAppEventHandler
	{
		virtual void BindMainWindow(HWND hWnd) = 0;
		virtual bool OnClose() = 0;
		virtual void OnKeyboardEvent(const RAWKEYBOARD& k) = 0;
		virtual void OnMouseEvent(const RAWMOUSE& m) = 0;
		virtual void OnSize(HWND hWnd, const Vec2i& span, RESIZE_TYPE type) = 0;
	};

	struct IGuiRenderer
	{
		virtual void AddTriangle(const GuiVertex triangle[3]) = 0;
		virtual void FlushLayer() = 0;

		virtual Vec2i EvalSpan(const Vec2i& pos, Fonts::IDrawTextJob& job) = 0;
		virtual void RenderText(const Vec2i& pos, Fonts::IDrawTextJob& job) = 0;
	};

	struct IScene
	{
		virtual RGBA GetClearColour() const = 0;
		virtual void RenderGui(IGuiRenderer& gr) = 0;
	};

	struct GuiMetrics
	{
		Vec2i cursorPosition;
		Vec2i screenSpan;
	};

	struct IRenderer
	{
		virtual ID_VERTEX_SHADER CreateVertexShader(const wchar_t* name, const byte* shaderCode, size_t shaderLength) = 0;
		virtual ID_PIXEL_SHADER CreatePixelShader(const wchar_t* name, const byte* shaderCode, size_t shaderLength) = 0;

		virtual void Render(IScene& scene) = 0;
		virtual void UseShaders(ID_VERTEX_SHADER vid, ID_PIXEL_SHADER pid) = 0;
		virtual void GetGuiMetrics(GuiMetrics& metrics) const = 0;
	};

	struct IGuiVertex
	{
		virtual void AddTriangle(const GuiVertex triangle[3]) = 0;
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
			ExpandZoneToContain(renderZone, Vec2(p.x, p.y));
			ExpandZoneToContain(renderZone, Vec2(p.x + dx, p.y + dy));
		}

		Vec2 Span() const
		{
			return Rococo::Span(renderZone);
		}
	};

	class HorizontalCentredText : public Fonts::IDrawTextJob
	{
	private:
		const wchar_t* text;
		Fonts::FontColour colour;
		int fontIndex;

	public:
		Quad target;

		HorizontalCentredText(int _fontIndex, const wchar_t* _text, Fonts::FontColour _colour) : text(_text), colour(_colour), fontIndex(_fontIndex)
		{
			float minFloat = std::numeric_limits<float>::min();
			float maxFloat = std::numeric_limits<float>::max();
			target = Quad(maxFloat, maxFloat, minFloat, minFloat);
		}

		void DrawNextGlyph(char c, Fonts::IGlyphBuilder& builder)
		{
			Quad outputRect;
			builder.AppendChar(c, outputRect);

			if (outputRect.left < target.left) target.left = outputRect.left;
			if (outputRect.right > target.right) target.right = outputRect.right;
			if (outputRect.bottom > target.bottom) target.bottom = outputRect.bottom;
			if (outputRect.top < target.top) target.top = outputRect.top;
		}

		virtual void OnDraw(Fonts::IGlyphBuilder& builder)
		{
			builder.SetTextColour(colour);
			builder.SetShadow(false);
			builder.SetFontIndex(fontIndex);

			float fMin = -1000000.0f;
			float fMax = 1000000.0f;
			builder.SetClipRect(Quad(fMin, fMin, fMax, fMax));

			for (const wchar_t* p = text; *p != 0; p++)
			{
				wchar_t c = *p;

				if (c >= 255) c = '?';

				if (c == '\t')
				{
					for (int i = 0; i < 4; ++i)
					{
						DrawNextGlyph(' ', builder);
					}
				}
				else
				{
					DrawNextGlyph((char)c, builder);
				}
			}
		}
	};

	struct GuiScale
	{
		float OOScreenWidth;
		float OOScreenHeight;
		float OOFontWidth;
		float OOFontHeight;
	};

	class DX11AppRenderer : public IAppEventHandler, public IRenderer, public IGuiRenderer, public Fonts::IGlyphRenderer
	{
	private:
		ID3D11Device& device;
		ID3D11DeviceContext& dc;
		IDXGIFactory& factory;

		AutoRelease<IDXGISwapChain> mainSwapChain;
		AutoRelease<ID3D11RenderTargetView> mainBackBufferView;

		std::vector<DX11VertexShader*> vertexShaders;
		std::vector<DX11PixelShader*> pixelShaders;
		AutoRelease<ID3D11Buffer> guiBuffer;

		std::vector<GuiVertex> guiVertices;

		enum { GUI_BUFFER_VERTEX_CAPACITY = 3 * 512 };

		AutoRelease<ID3D11Texture2D> fontTexture;
		AutoRelease<ID3D11SamplerState> spriteSampler;
		AutoRelease<ID3D11ShaderResourceView> fontBinding;
		AutoRelease<ID3D11RasterizerState> spriteRendering;
		AutoRelease<ID3D11BlendState> alphaBlend;

		Fonts::IFontSupervisor* fonts;
		Quad clipRect;

		GuiScale guiScaleVector;
		AutoRelease<ID3D11Buffer> guiScaleBuffer;

		RAWMOUSE lastMouseEvent;
		Vec2i screenSpan;

		HWND hRenderWindow;
	public:
		DX11AppRenderer(ID3D11Device& _device, ID3D11DeviceContext& _dc, IDXGIFactory& _factory):
			device(_device), dc(_dc), factory(_factory), fonts(nullptr), clipRect(-10000.0f, -10000.0f, 10000.0f, 10000.0f), hRenderWindow(0)
		{
			D3D11_BUFFER_DESC bufferDesc;
			bufferDesc.ByteWidth = sizeof(GuiVertex) * GUI_BUFFER_VERTEX_CAPACITY;
			bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
			bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
			bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			bufferDesc.MiscFlags = 0;
			bufferDesc.StructureByteStride = 0;

			VALIDATEDX11(device.CreateBuffer(&bufferDesc, nullptr, guiBuffer));

			static_assert(GUI_BUFFER_VERTEX_CAPACITY % 3 == 0, "Capacity must be divisible by 3");

			D3D11_SAMPLER_DESC spriteSamplerDesc;
			ZeroMemory(&spriteSamplerDesc, sizeof(spriteSamplerDesc));
			spriteSamplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
			spriteSamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
			spriteSamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
			spriteSamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
			spriteSamplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
			spriteSamplerDesc.MinLOD = 0;
			spriteSamplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

			VALIDATEDX11(device.CreateSamplerState(&spriteSamplerDesc, spriteSampler));

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

			VALIDATEDX11(device.CreateRasterizerState(&spriteRenderingDesc, spriteRendering));

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
			VALIDATEDX11(device.CreateBlendState(&alphaBlendDesc, alphaBlend));

			std::vector<byte> fontFile;
			const wchar_t* fontName = L"C:\\Work\\rococo\\dx11.window.test\\font1.tif";
			LoadFileAbsolute(fontName, fontFile, 64 * 1024 * 1024);

			if (fontFile.empty()) Throw(0, L"The font file %s was blank", fontName);

			struct ANON: public Imaging::IImageLoadEvents
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

					VALIDATEDX11(device->CreateTexture2D(&alphaImmutableSprite, &level0Def, fontTexture));
				}
			} anon(fontTexture);

			anon.fontName = fontName;
			anon.device = &device;

			Rococo::Imaging::DecompressTiff(anon, &fontFile[0], fontFile.size());

			D3D11_SHADER_RESOURCE_VIEW_DESC desc;
			ZeroMemory(&desc, sizeof(desc));

			desc.Texture2D.MipLevels = 1;
			desc.Texture2D.MostDetailedMip = 0;

			desc.Format = DXGI_FORMAT_UNKNOWN;
			desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;

			VALIDATEDX11(device.CreateShaderResourceView(fontTexture, &desc, fontBinding));

			const wchar_t* csvName = L"C:\\Work\\rococo\\dx11.window.test\\font1.csv";
			LoadFileAbsolute(csvName, fontFile, 65536);

			fonts = Fonts::LoadFontCSV(csvName, (const char*) &fontFile[0], fontFile.size());

			D3D11_BUFFER_DESC cbDesc;
			cbDesc.ByteWidth = sizeof(GuiScale);
			cbDesc.Usage = D3D11_USAGE_DYNAMIC;
			cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			cbDesc.MiscFlags = 0;
			cbDesc.StructureByteStride = 0;

			GuiScale scale{ 0,0,0,0 };

			// Fill in the subresource data.
			D3D11_SUBRESOURCE_DATA blank;
			blank.pSysMem = &scale;
			blank.SysMemPitch = 0;
			blank.SysMemSlicePitch = 0;

			// Create the buffer.
			VALIDATEDX11(device.CreateBuffer(&cbDesc, &blank, guiScaleBuffer));
		}

		~DX11AppRenderer()
		{
			ClearContext();

			if (fonts)
			{
				fonts->Free();
			}

			for (auto& x : vertexShaders)
			{
				delete x;
			}

			for (auto& x : pixelShaders)
			{
				delete x;
			}
		}

		void BindMainWindow(HWND hRenderWindow)
		{
			this->hRenderWindow = hRenderWindow;

			DXGI_SWAP_CHAIN_DESC swapChainDesc = GetSwapChainDescription(hRenderWindow);
			VALIDATEDX11(factory.CreateSwapChain((ID3D11Device*) &device, &swapChainDesc, mainSwapChain));

			AutoRelease<ID3D11Texture2D> backBuffer;
			VALIDATEDX11(mainSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)(ID3D11Texture2D**)backBuffer));

			VALIDATEDX11(device.CreateRenderTargetView(backBuffer, nullptr, mainBackBufferView));

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

			screenSpan.x = (int32) viewport.Width;
			screenSpan.y = (int32) viewport.Height;
		}

		ID_VERTEX_SHADER CreateVertexShader(const wchar_t* name, const byte* shaderCode, size_t shaderLength)
		{
			if (name == nullptr || wcslen(name) > 1024) Throw(0, L"Bad <name> for vertex shader");
			if (shaderCode == nullptr || shaderLength < 4 || shaderLength > 65536) Throw(0, L"Bad shader code for vertex shader %s", name);

			DX11VertexShader* shader = new DX11VertexShader;
			HRESULT hr = device.CreateInputLayout(GetGuiVertexDesc(), NumberOfVertexElements(), shaderCode, shaderLength, shader->inputLayout);
			if FAILED(hr)
			{
				delete shader;
				Throw(hr, L"device.CreateInputLayout failed with shader %s", name);
				return 0;
			}

			hr = device.CreateVertexShader(shaderCode, shaderLength, nullptr, shader->vs);
			if FAILED(hr)
			{
				delete shader;
				Throw(hr, L"device.CreateVertexShader failed with shader %s", name);
				return 0;
			}

			shader->name = name;
			vertexShaders.push_back(shader);
			return (ID_VERTEX_SHADER) vertexShaders.size() - 1;
		}

		ID_PIXEL_SHADER CreatePixelShader(const wchar_t* name, const byte* shaderCode, size_t shaderLength)
		{
			if (name == nullptr || wcslen(name) > 1024) Throw(0, L"Bad <name> for pixel shader");
			if (shaderCode == nullptr || shaderLength < 4 || shaderLength > 16384) Throw(0, L"Bad shader code for pixel shader %s", name);

			DX11PixelShader* shader = new DX11PixelShader;
			HRESULT hr = device.CreatePixelShader(shaderCode, shaderLength, nullptr, shader->ps);
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
		}

		virtual bool OnClose()
		{
			BOOL isFullScreen;
			AutoRelease<IDXGIOutput> output;
			if SUCCEEDED(mainSwapChain->GetFullscreenState(&isFullScreen, output))
			{
				if (isFullScreen)
				{
					mainSwapChain->SetFullscreenState(false, nullptr);
				}
			}
			return true;
		}

		virtual void OnKeyboardEvent(const RAWKEYBOARD& k)
		{

		}

		virtual void OnMouseEvent(const RAWMOUSE& m)
		{
			lastMouseEvent = m;
		}

		virtual void GetGuiMetrics(GuiMetrics& metrics) const
		{
			POINT p;
			GetCursorPos(&p);
			ScreenToClient(hRenderWindow, &p);
			
			metrics.cursorPosition = Vec2i(p.x, p.y);
			metrics.screenSpan = screenSpan;
		}

		void Render(IScene& scene)
		{
			if (mainBackBufferView.IsNull()) return;
			
			dc.OMSetRenderTargets(1, mainBackBufferView, nullptr);

			RGBA clearColour = scene.GetClearColour();

			if (clearColour.alpha > 0)
			{
				dc.ClearRenderTargetView(mainBackBufferView, (const FLOAT*)&clearColour);
			}
			
			dc.IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			
			UINT stride = sizeof(GuiVertex);
			UINT offsets = 0;
			dc.IASetVertexBuffers(0, 1, guiBuffer, &stride, &offsets);

			dc.PSSetSamplers(0, 1, spriteSampler);
			dc.PSSetShaderResources(0, 1, fontBinding);

			dc.RSSetState(spriteRendering);

			FLOAT blendFactorUnused[] = { 0,0,0,0 };
			dc.OMSetBlendState(alphaBlend, blendFactorUnused, 0xffffffff);

			guiScaleVector.OOScreenWidth = 1.0f / screenSpan.x;
			guiScaleVector.OOScreenHeight = 1.0f / screenSpan.y;
			guiScaleVector.OOFontWidth = fonts->TextureSpan().z;
			guiScaleVector.OOFontHeight = fonts->TextureSpan().w;

			D3D11_MAPPED_SUBRESOURCE bufferMap;
			VALIDATEDX11(dc.Map(guiScaleBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &bufferMap));

			GuiScale* dest = (GuiScale*) bufferMap.pData;
			*dest = guiScaleVector;

			dc.Unmap(guiScaleBuffer, 0);

			dc.VSSetConstantBuffers(0, 1, guiScaleBuffer);

			scene.RenderGui(*this);

			FlushLayer();

			dc.PSSetShaderResources(0, 0, nullptr);
			dc.PSSetSamplers(0, 0, nullptr);
			
			mainSwapChain->Present(0, 0);

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

				dc.Draw((UINT) chunk, 0);

				nVerticesLeftToRender -= chunk;
			};

			guiVertices.clear();
		}

		void ResizeBuffers(const Vec2i& span)
		{
			mainBackBufferView.Detach();

			VALIDATEDX11(mainSwapChain->ResizeBuffers(1, span.x, span.y, DXGI_FORMAT_R8G8B8A8_UNORM, 0));

			AutoRelease<ID3D11Texture2D> backBuffer;
			VALIDATEDX11(mainSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)(ID3D11Texture2D**)backBuffer));
			VALIDATEDX11(device.CreateRenderTargetView(backBuffer, nullptr, mainBackBufferView));

			D3D11_VIEWPORT viewport;
			ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));

			viewport.TopLeftX = 0;
			viewport.TopLeftY = 0;
			viewport.Width = FLOAT(span.x);
			viewport.Height = FLOAT(span.y);
			viewport.MinDepth = 0.0f;
			viewport.MaxDepth = 1.0f;
			dc.RSSetViewports(1, &viewport);

			screenSpan.x = (int32)viewport.Width;
			screenSpan.y = (int32)viewport.Height;
		}

		virtual void OnSize(HWND hWnd, const Vec2i& span, RESIZE_TYPE type)
		{
			if (span.x > 0 && span.y > 0)
			{
				D3D11_TEXTURE2D_DESC desc;
				{
					AutoRelease<ID3D11Texture2D> backBuffer;
					VALIDATEDX11(mainSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)(ID3D11Texture2D**)backBuffer));
					backBuffer->GetDesc(&desc);
				}

				if (desc.Width != span.x || desc.Height != span.y)
				{
					ResizeBuffers(span);
				}
			}
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
			SetOverlappedWindowConfig(config, Vec2i(800, 600), SW_SHOWMAXIMIZED, nullptr, L"DX11 64-bit Rococo API Window", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 0);
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
			UINT sizeofBuffer = 0;
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
}

void Create_DX11_0_DebugHost(UINT adapterIndex, DX11Host& host)
{
	VALIDATEDX11(CreateDXGIFactory(IID_IDXGIFactory, (void**)(IDXGIFactory**)host.factory));
	VALIDATEDX11(host.factory->EnumAdapters(adapterIndex, host.adapter));

	D3D_FEATURE_LEVEL featureLevelNeeded[] = { D3D_FEATURE_LEVEL_11_0 };
	D3D_FEATURE_LEVEL featureLevelFound;

	VALIDATEDX11(D3D11CreateDevice(host.adapter, D3D_DRIVER_TYPE_UNKNOWN, nullptr, D3D11_CREATE_DEVICE_DEBUG,
		featureLevelNeeded, 1, D3D11_SDK_VERSION, host.device, &featureLevelFound, host.dc
		));

	if (featureLevelFound != D3D_FEATURE_LEVEL_11_0)
	{
		Throw(0, L"DX 11.0 is required for this application");
	}
}

class DX11App: public IScene
{
private:
	IRenderer& renderer;

public:
	DX11App(IRenderer& _renderer) : renderer(_renderer) {}

	void InitGraphics()
	{
		std::vector<byte> shaderCode(65536);
		LoadFileAbsolute(L"C:\\Work\\rococo\\dx11.window.test\\vshader.cso", shaderCode, 65536);
		auto vid = renderer.CreateVertexShader(L"shader1.vb", &shaderCode[0], shaderCode.size());

		LoadFileAbsolute(L"C:\\Work\\rococo\\dx11.window.test\\pshader.cso", shaderCode, 65536);
		auto pid = renderer.CreatePixelShader(L"shader1.pb", &shaderCode[0], shaderCode.size());

		renderer.UseShaders(vid, pid);
	}

	virtual uint32 OnTick(const IUltraClock& clock)
	{
		renderer.Render(*this);
		return 5;
	}

	RGBA GetClearColour() const
	{
		return RGBA(0.0f, 0.0f, 0.0f, 1.0f);
	}

	Fonts::FontColour FontColourFromRGBAb(RGBAb colour)
	{
		Fonts::FontColour* pCol = (Fonts::FontColour*) &colour;
		return *pCol;
	}

	void RenderGui(IGuiRenderer& gr)
	{
		GuiVertex q0[6] =
		{
			{   0.0f,600.0f, 1.0f, 0.0f,{   255,0,0,255 }, 0.0f, 0.0f, 0 }, // bottom left
			{ 800.0f,600.0f, 1.0f, 0.0f,{   0,255,0,255 }, 0.0f, 0.0f, 0 }, // bottom right
			{   0.0f,  0.0f, 1.0f, 0.0f,{   0,0,255,255 }, 0.0f, 0.0f, 0 }, // top left
			{   0.0f,  0.0f, 1.0f, 0.0f,{ 255,255,0,255 }, 0.0f, 0.0f, 0 }, // top left
			{ 800.0f,600.0f, 1.0f, 0.0f,{ 0,255,255,255 }, 0.0f, 0.0f, 0 }, // bottom right
			{ 800.0f,  0.0f, 1.0f, 0.0f,{ 255,0,255,255 }, 0.0f, 0.0f, 0 }  // top right
		};

		gr.AddTriangle(q0);
		gr.AddTriangle(q0 + 3);

		GuiMetrics metrics;
		renderer.GetGuiMetrics(metrics);

		wchar_t info[256];
		SafeFormat(info, _TRUNCATE, L"Mouse: (%d,%d)", metrics.cursorPosition.x, metrics.cursorPosition.y);

		HorizontalCentredText hw(0, info, FontColourFromRGBAb(RGBAb{ 255, 255, 255, 255 }));
		gr.RenderText(Vec2i(25, 25), hw);
	}
};

void MainLoop(MainWindowHandler& mainWindow, HANDLE hInstanceLock, DX11App& app)
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

		QueryPerformanceCounter((LARGE_INTEGER*) &uc.tickStart);

		uc.tickDelta = uc.tickStart - lastTick;

		sleepMS = app.OnTick(uc);

		lastTick = uc.tickStart;
	}
}

void Main(HANDLE hInstanceLock)
{
	DX11Host host;
	Create_DX11_0_DebugHost(0, host);
	
	DX11AppRenderer renderer(*host.device, *host.dc, *host.factory);

	DX11App app(renderer);

	AutoFree<MainWindowHandler> mainWindowHandler(MainWindowHandler::Create(renderer));
	SetWindowText(mainWindowHandler->Window(), L"DX11 Main Window");
	VALIDATEDX11(host.factory->MakeWindowAssociation(mainWindowHandler->Window(), 0));

	app.InitGraphics();

	MainLoop(*mainWindowHandler, hInstanceLock, app);
}

int CALLBACK WinMain(HINSTANCE _hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	using namespace Rococo;
	using namespace Rococo::Windows;

	HANDLE hInstanceLock = CreateEvent(nullptr, TRUE, FALSE, L"RococoApp_InstanceLock");

	if (GetLastError() == ERROR_ALREADY_EXISTS)
	{
		SetEvent(hInstanceLock);

		if (IsDebuggerPresent())
		{
			MessageBox(nullptr, L"Application is already running", L"Rococo App is single instance only", MB_ICONEXCLAMATION);
		}
		return -1;
	}

	try
	{
		InitRococoWindows(_hInstance, LoadIcon(_hInstance, MAKEINTRESOURCE(IDI_ICON1)), LoadIcon(_hInstance, MAKEINTRESOURCE(IDI_ICON1)), nullptr, nullptr); // This must be called once, in WinMain or DllMain
		Main(hInstanceLock);
	}
	catch (IException& ex)
	{
		ShowErrorBox(ex, L"DX11 64-bit Rococo API Window threw an exception");
	}

	CloseHandle(hInstanceLock);

	return 0;
}
