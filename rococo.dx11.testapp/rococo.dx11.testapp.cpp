#include <rococo.api.h>
#include <rococo.os.win32.h>
#include <rococo.libs.inl>
#include <dxgi.h>
#include <rococo.window.h>
#include <rococo.strings.h>
#include <rococo.os.h>
#include <rococo.renderer.h>
#include <rococo.strings.h>
#include <vector>
#include <rococo.mplat.h>
#include <rococo.fonts.hq.h>
#include <rococo.DirectX.h>

#include <sexy.lib.s-parser.h>
#include <sexy.lib.util.h>
#include <sexy.lib.script.h>
#include <rococo.auto-release.h>

#ifdef _DEBUG
# pragma comment(lib, "rococo.windows.debug.lib")
# pragma comment(lib, "rococo.fonts.debug.lib")
# pragma comment(lib, "rococo.mplat.debug.lib")
# pragma comment(lib, "rococo.sexy.ide.debug.lib")
# pragma comment(lib, "dx11.deferred.renderer.debug.lib")
# pragma comment(lib, "rococo.DirectX.debug.lib")
#else
# pragma comment(lib, "rococo.windows.lib")
# pragma comment(lib, "rococo.fonts.lib")
# pragma comment(lib, "rococo.mplat.lib")
# pragma comment(lib, "rococo.sexy.ide.lib")
# pragma comment(lib, "dx11.deferred.renderer.lib")
# pragma comment(lib, "rococo.DirectX.lib")
#endif

#include "..\dx11.deferred.renderer\rococo.dx11.h"

#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "D3DCompiler.lib")

using namespace Rococo;
using namespace Rococo::Graphics;

void Main(HINSTANCE hInstance)
{
	Rococo::OS::SetBreakPoints(Rococo::OS::BreakFlag_All);

	AutoFree<IOSSupervisor> os = GetOS();
	AutoFree<IInstallationSupervisor> installation = CreateInstallation(L"content.indicator.txt", *os);
	AutoFree<IWin32AdapterContext> ac = CreateWin32AdapterContext(0, 0);
	AutoFree<IDX11System> dx11 = CreateDX11System(ac->AC(), *installation);

	WideFilePath shaderPath;
	installation->ConvertPingPathToSysPath("!shaders/", shaderPath);
	os->Monitor(shaderPath);

	AutoFree<IPipelineSupervisor> pipeline = CreatePipeline(*dx11);

	struct CLOSURE : IDX1RendererWindowEventHandler
	{
		bool isRunning = true;
		int err = 0;
		HString msg;

		IDX11System& system;
		IPipelineSupervisor& pipeline;

		TextureId idBackBuffer;
		TextureId idDepthBuffer;

		CLOSURE(IPipelineSupervisor& ref_pipeline, IDX11System& ref_system):
			system(ref_system), pipeline(ref_pipeline)
		{

		}

		void OnCloseRequested(IDX11Window& window) override
		{
			isRunning = false;
		}

		void OnMessageQueueException(IDX11Window& window, IException& ex) override
		{
			err = ex.ErrorCode();
			msg = ex.Message();
		}

		void OnKeyboardEvent(const RAWKEYBOARD& k, HKL hKeyboardLayout) override
		{

		}

		void OnMouseEvent(const RAWMOUSE& m) override
		{

		}

		void OnResizeBackBuffer(TextureId idBackBuffer, Vec2i span) override
		{
			system.Textures().UpdateSpanFromSystem(idBackBuffer);

			this->idBackBuffer = idBackBuffer;
			this->idDepthBuffer = system.Textures().AddDepthStencil("depth", span, 32, 0);
		}

		void OnUpdateFrame(TextureId idBackBuffer) override
		{
			pipeline.Execute();
		}

		void ThrowOnError()
		{
			if (err != 0 || msg.length() != 0)
			{
				Throw(err, msg);
			}
		}
	} app(*pipeline, *dx11);

	struct Scene : IScene
	{
	public:
		void GetCamera(Matrix4x4& camera, Matrix4x4& world, Matrix4x4& proj, Vec4& eye, Vec4& viewDir) override
		{

		}

		ID_CUBE_TEXTURE GetSkyboxCubeId() const override
		{

		}

		RGBA GetClearColour() const override
		{

		}

		void OnGuiResize(Vec2i screenSpan) override
		{

		}

		void RenderGui(IGuiRenderContext& grc) override
		{
			GuiMetrics metrics;
			grc.Renderer().GetGuiMetrics(metrics);
			int32 y = metrics.screenSpan.y >> 1;

		//	GuiRect rect{ 0, metrics.screenSpan.x, y - 100, y + 100 };
		//	RenderHQText_LeftAligned_VCentre(grc, fontId, rect, "Hello World!", RGBAb(255, 255, 255, 255));
		}

		void RenderObjects(IRenderContext& rc, bool skinned) override
		{

		}

		const Light* GetLights(uint32& nCount) const override
		{
			nCount = 0;
			return nullptr;
		}

		 void RenderShadowPass(const DepthRenderData& drd, IRenderContext& rc, bool skinned) override
		 {

		 }
	};

	DX11WindowContext wc{ Windows::NoParent(), app, {800,600}, "DX11 Test Window", hInstance };
	AutoFree<IDX11Window> window = dx11->CreateDX11Window(wc);
	window->MonitorShaderErrors(&dx11->Shaders());
	auto idScalableFontTexture = dx11->Textures().AddTx2D_Grey("!font1.tif");
	dx11->Textures().ReloadAsset(idScalableFontTexture);

	auto idMats = dx11->Textures().AddTx2DArray_RGBAb("materials", { 1024,1024 });
	dx11->Textures().AddElementToArray(idMats, "!textures/hv/materials/hi-rez/marble/blue.jpg");
	dx11->Textures().AddElementToArray(idMats, "!textures/hv/materials/hi-rez/marble/brown.1.jpg");
	dx11->Textures().EnableMipMapping(idMats);
	dx11->Textures().ReloadAsset(idMats);

	auto idSprites = dx11->Textures().AddTx2D_UVAtlas("sprites");
	dx11->Textures().AddElementToArray(idSprites, "!textures/hv/icons/bastard.sword.tif");
	dx11->Textures().AddElementToArray(idSprites, "!textures/hv/icons/mace.tif");
	dx11->Textures().ReloadAsset(idSprites);

	AutoRelease<IRenderStageSupervisor> guiStage = CreateRenderStageBasic(*dx11);

	RECT rect;
	GetClientRect(window->Window(), &rect);
	GuiRect guiRect{ 0, 0, rect.right, rect.bottom };
	guiStage->SetEnableScissors(&guiRect);

	guiStage->SetDepthWriteEnable(false);
	guiStage->SetEnableDepth(false);
	guiStage->SetSrcBlend(BlendValue::SRC_ALPHA);
	guiStage->SetDestBlend(BlendValue::INV_SRC_ALPHA);
	guiStage->SetEnableBlend(true);
	guiStage->SetBlendOp(BlendOp::ADD);
	guiStage->SetCullMode(0);

	Sampler spriteDef;
	spriteDef.filter = SamplerFilter::MIN_MAG_MIP_POINT;
	spriteDef.maxAnisotropy = 1;
	spriteDef.comparison = ComparisonFunc::ALWAYS;
	spriteDef.maxLOD = 1000000.0f;
	spriteDef.minLOD = 0;
	spriteDef.mipLODBias = 0;
	spriteDef.U = SamplerAddressMode::BORDER;
	spriteDef.V = SamplerAddressMode::BORDER;
	spriteDef.W = SamplerAddressMode::BORDER;
	guiStage->AddInput(idSprites, 7, spriteDef);

	Sampler fontDef;
	fontDef.filter = SamplerFilter::MIN_MAG_MIP_LINEAR;
	fontDef.maxAnisotropy = 1;
	fontDef.comparison = ComparisonFunc::ALWAYS;
	fontDef.maxLOD = 1000000.0f;
	fontDef.minLOD = 0;
	fontDef.mipLODBias = 0;
	fontDef.U = SamplerAddressMode::BORDER;
	fontDef.V = SamplerAddressMode::BORDER;
	fontDef.W = SamplerAddressMode::BORDER;
	guiStage->AddInput(idScalableFontTexture, 0, fontDef);

	AutoFree<IRenderPhasePopulator> gui = CreateStandardGuiRenderPhase(*dx11, *guiStage);
	guiStage->SetPopulator(gui);

	RenderTargetFlags flags;
	flags.clearWhenAssigned = true;
	guiStage->AddOutput(app.idBackBuffer,  0, 0, flags, RGBA{ 0, 1, 0, 1});
	guiStage->AddDepthStencilBuffer(app.idDepthBuffer, 1.0f, 0);

	pipeline->GetBuilder().AddStage("gui", guiStage);

	auto start = Rococo::OS::CpuTicks();

	MSG msg;
	while (app.isRunning)
	{
		MsgWaitForMultipleObjects(0, NULL, FALSE, 100, QS_ALLINPUT);

		struct CLOSURE : IEventCallback<FileModifiedArgs>
		{
			IShaderCache* shaders;

			void OnEvent(FileModifiedArgs& args) override
			{
				if (EndsWith(args.sysPath, L".hlsl"))
				{
					shaders->ReloadShader(args.sysPath);
				}
			}
		} dispatchChanges;
		dispatchChanges.shaders = &dx11->Shaders();
		os->EnumerateModifiedFiles(dispatchChanges);

		window->UpdateFrame();

		while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessageA(&msg);
		}

		Rococo::OS::ticks now = Rococo::OS::CpuTicks();

		enum { TIME_OUT_SECONDS = 120 };
		if (now - start > Rococo::OS::CpuHz() * TIME_OUT_SECONDS)
		{
			break;
		}
	}

	app.ThrowOnError();
}

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	try
	{
		Main(hInstance);
		return 0;
	}
	catch (IException& ex)
	{
		Rococo::OS::ShowErrorBox(Rococo::Windows::NoParent(), ex, "DX11 Deferred Renderer Test App");
		return ex.ErrorCode();
	}
}
